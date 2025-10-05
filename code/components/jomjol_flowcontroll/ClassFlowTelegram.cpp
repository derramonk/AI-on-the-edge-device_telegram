#ifdef ENABLE_TELEGRAM
#include <sstream>
#include "ClassFlowTelegram.h"
#include "Helper.h"
#include "connect_wlan.h"
#include "time_sntp.h"
#include "interface_telegram.h"
#include "ClassFlowPostProcessing.h"
#include "ClassFlowAlignment.h"
#include "esp_log.h"
#include "../../include/defines.h"
#include "ClassLogFile.h"
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "TELEGRAM_FLOW";

// Static members for task handling
TaskHandle_t ClassFlowTelegram::telegramTaskHandle = nullptr;
ClassFlowTelegram* ClassFlowTelegram::instance = nullptr;

void ClassFlowTelegram::SetInitialParameter(void)
{
    botToken = "";
    chatId = "";
    flowpostprocessing = NULL;
    flowAlignment = NULL;
    previousElement = NULL;
    ListFlowControll = NULL; 
    disabled = false;
    TelegramEnable = false;
    TelegramUploadImg = 0;
    TelegramOnError = false;
    TelegramBotCommands = false;
}

ClassFlowTelegram::ClassFlowTelegram()
{
    SetInitialParameter();
    instance = this; // Set static instance
}

ClassFlowTelegram::ClassFlowTelegram(std::vector<ClassFlow*>* lfc)
{
    SetInitialParameter();
    instance = this; // Set static instance
    ListFlowControll = lfc;
    for (int i = 0; i < ListFlowControll->size(); ++i)
    {
        if (((*ListFlowControll)[i])->name().compare("ClassFlowPostProcessing") == 0)
        {
            flowpostprocessing = (ClassFlowPostProcessing*) (*ListFlowControll)[i];
        }
        if (((*ListFlowControll)[i])->name().compare("ClassFlowAlignment") == 0)
        {
            flowAlignment = (ClassFlowAlignment*) (*ListFlowControll)[i];
        }
    }
}

ClassFlowTelegram::ClassFlowTelegram(std::vector<ClassFlow*>* lfc, ClassFlow *_prev)
{
    SetInitialParameter();
    instance = this; // Set static instance
    previousElement = _prev;
    ListFlowControll = lfc;

    for (int i = 0; i < ListFlowControll->size(); ++i)
    {
        if (((*ListFlowControll)[i])->name().compare("ClassFlowPostProcessing") == 0)
        {
            flowpostprocessing = (ClassFlowPostProcessing*) (*ListFlowControll)[i];
        }
        if (((*ListFlowControll)[i])->name().compare("ClassFlowAlignment") == 0)
        {
            flowAlignment = (ClassFlowAlignment*) (*ListFlowControll)[i];
        }
    }
}

ClassFlowTelegram::~ClassFlowTelegram()
{
    // Clean up the task when the object is destroyed
    stopTelegramTask();
    if (instance == this) {
        instance = nullptr;
    }
}

bool ClassFlowTelegram::ReadParameter(FILE* pfile, string& aktparamgraph)
{
    std::vector<string> zerlegt;

    aktparamgraph = trim(aktparamgraph);
    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "ReadParameter called with aktparamgraph: '" + aktparamgraph + "'");

    if (aktparamgraph.size() == 0)
    {
        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "aktparamgraph is empty, calling getNextLine");
        if (!this->getNextLine(pfile, &aktparamgraph))
        {
            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "getNextLine returned false");
            return false;
        }
        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "getNextLine returned: '" + aktparamgraph + "'");
    }

    if (aktparamgraph.compare("[Telegram]") != 0)
    {
        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "aktparamgraph is not [Telegram], it is: '" + aktparamgraph + "'");
        return false;
    }

    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Found [Telegram] section, starting to parse parameters");

    while (this->getNextLine(pfile, &aktparamgraph) && !this->isNewParagraph(aktparamgraph))
    {
        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Inside while loop - aktparamgraph: '" + aktparamgraph + "'");
        zerlegt = ZerlegeZeile(aktparamgraph);
        std::string _param = GetParameterName(zerlegt[0]);
        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Parsing line: '" + aktparamgraph + "' -> Key: '" + _param + "', Value: '" + (zerlegt.size() > 1 ? zerlegt[1] : "EMPTY") + "'");
        
        if ((toUpper(_param) == "TELEGRAMENABLE") && (zerlegt.size() > 1))
        {
            if (toUpper(zerlegt[1]) == "TRUE")
                TelegramEnable = true;
            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "TelegramEnable set to: " + std::to_string(TelegramEnable));
        }
        if ((toUpper(_param) == "BOTTOKEN") && (zerlegt.size() > 1))
        {
            botToken = zerlegt[1];
            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "BotToken set to: " + botToken);
        }
        if ((toUpper(_param) == "CHATID") && (zerlegt.size() > 1))
        {
            chatId = zerlegt[1];
            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "ChatID set to: " + chatId);
        }
        if ((toUpper(_param) == "TELEGRAMUPLOADIMG") && (zerlegt.size() > 1))
        {
            TelegramUploadImg = std::stoi(zerlegt[1]);
            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "TelegramUploadImg set to: " + std::to_string(TelegramUploadImg));
        }
        if ((toUpper(_param) == "TELEGRAMERROR") && (zerlegt.size() > 1))
        {
            if (toUpper(zerlegt[1]) == "TRUE")
                TelegramOnError = true;
            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "TelegramOnError set to: " + std::to_string(TelegramOnError));
        }
        if ((toUpper(_param) == "TELEGRAMBOTCOMMANDS") && (zerlegt.size() > 1))
        {
            if (toUpper(zerlegt[1]) == "TRUE")
                TelegramBotCommands = true;
            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "TelegramBotCommands set to: " + std::to_string(TelegramBotCommands));
        }
    }

    // Don't initialize Telegram during boot phase to avoid conflicts with camera init
    // Will be initialized on first doFlow() call
    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Telegram configuration loaded - TelegramEnable: " + std::to_string(TelegramEnable) + 
                       ", BotCommands: " + std::to_string(TelegramBotCommands) +
                       ", BotToken: " + (botToken.empty() ? "EMPTY" : "SET") + 
                       ", ChatID: " + (chatId.empty() ? "EMPTY" : "SET"));

    // Task start disabled to prevent camera instability
    // LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Starting Telegram task early");
    // startTelegramTask();

    return true;
}

bool ClassFlowTelegram::doFlow(string zwtime)
{
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "doFlow called - TelegramEnable: " + std::to_string(TelegramEnable));
    
    if (!TelegramEnable)
        return true;

    if (botToken.empty() || chatId.empty())
    {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Telegram bot token or chat ID not configured");
        return false;
    }

    // Initialize Telegram on first use (lazy initialization)
    static bool telegramInitialized = false;
    if (!telegramInitialized) {
        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Initializing Telegram for first use");
        TelegramInit(botToken, chatId);
        
        // Task start disabled to prevent camera instability
        // LogFile.WriteToFile(ESP_LOG_INFO, TAG, "startTelegramTask");
        // Start the Telegram bot command listener
        // startTelegramTask();
        
        telegramInitialized = true;
    }

    // Debug: Check if flowpostprocessing is available
    if (!flowpostprocessing) {
        LogFile.WriteToFile(ESP_LOG_INFO, TAG, "flowpostprocessing is NULL - searching in available flows:");
        if (ListFlowControll) {
            for (int i = 0; i < ListFlowControll->size(); ++i) {
                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Available flow " + std::to_string(i) + ": " + ((*ListFlowControll)[i])->name());
                // Find PostProcessing flow at runtime
                if (((*ListFlowControll)[i])->name().compare("ClassFlowPostProcessing") == 0)
                {
                    flowpostprocessing = (ClassFlowPostProcessing*) (*ListFlowControll)[i];
                    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Found and assigned flowpostprocessing at runtime");
                    break;
                }
            }
        } else {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "ListFlowControll is NULL");
        }
    }

    if (flowpostprocessing)
    {
        std::vector<NumberPost*>* numbers = flowpostprocessing->GetNumbers();
        bool hasError = false;
        
        // Check for errors
        for (int i = 0; i < numbers->size(); ++i)
        {
            if ((*numbers)[i]->ErrorMessage)
            {
                hasError = true;
                break;
            }
        }

        // Send message only on error if TelegramOnError is true, otherwise always send
        if (!TelegramOnError || hasError)
        {
            std::string message = "📊 AI-on-the-edge Update\n";
            message += "🕒 " + zwtime + "\n\n";

            // Limit number of numbers to prevent long messages
            int maxNumbers = (numbers->size() > 3) ? 3 : numbers->size();
            for (int i = 0; i < maxNumbers; ++i)
            {
                message += "📊 " + (*numbers)[i]->name + "\n";
                message += "📈 " + (*numbers)[i]->ReturnValue + "\n";
                
                if ((*numbers)[i]->ErrorMessage)
                {
                    message += "⚠️ " + (*numbers)[i]->ErrorMessageText + "\n";
                }
                message += "\n";
            }
            
            if (numbers->size() > 3) {
                message += "... and " + std::to_string(numbers->size() - 3) + " more values\n";
            }

            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Sending Telegram message");
            TelegramSendMessage(message);

            // Disable image upload for now to prevent memory issues
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Image upload disabled for stability");
        }
    }
       
    return true;
}

void ClassFlowTelegram::startTelegramTask()
{
    // Start the telegram task early if Telegram is enabled
    if (TelegramBotCommands && TelegramEnable)
    {
        if (telegramTaskHandle == nullptr)
        {
            ESP_LOGI(TAG, "Starting Telegram task");
            BaseType_t result = xTaskCreate(
                telegramTask,            // Task function
                "telegram_task",         // Task name
                1536,                    // Stack size (reduced from 4096)
                nullptr,                 // Parameter
                2,                       // Priority (reduced from 3)
                &telegramTaskHandle      // Task handle
            );
            
            if (result != pdPASS)
            {
                ESP_LOGE(TAG, "Failed to create Telegram task");
                telegramTaskHandle = nullptr;
            }
            else
            {
                ESP_LOGI(TAG, "Telegram task started successfully");
            }
        }
    }
}

void ClassFlowTelegram::stopTelegramTask()
{
    if (telegramTaskHandle != nullptr)
    {
        ESP_LOGI(TAG, "Stopping Telegram task");
        vTaskDelete(telegramTaskHandle);
        telegramTaskHandle = nullptr;
    }
}

void ClassFlowTelegram::telegramTask(void* parameter)
{
    ESP_LOGI(TAG, "Telegram task started - will log every 5 seconds");
    int counter = 0;
    while (telegramTaskHandle != nullptr)
    {
        // Log message every 10 seconds with counter (reduced frequency)
        counter++;
        ESP_LOGI(TAG, "Telegram Task #%d", counter);
        // Wait 10 seconds before next log (increased from 5 seconds)
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

#endif //ENABLE_TELEGRAM
