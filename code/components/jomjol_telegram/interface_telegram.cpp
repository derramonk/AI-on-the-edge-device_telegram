#ifdef ENABLE_TELEGRAM
#include "interface_telegram.h"

#include "esp_log.h"
#include "esp_http_client.h"
#include "ClassLogFile.h"
#include <cJSON.h>
#include "../../include/defines.h"

static const char *TAG = "TELEGRAM";

std::string _botToken;
std::string _chatId;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id)
    {
        case HTTP_EVENT_ERROR:
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP Client Error encountered");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP Client connected");
            break;
        case HTTP_EVENT_HEADERS_SENT:
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP Client sent all request headers");
            break;
        case HTTP_EVENT_ON_HEADER:
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Header: key=" + std::string(evt->header_key) + ", value=" + std::string(evt->header_value));
            break;
        case HTTP_EVENT_ON_DATA:
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP Client data received: len=" + std::to_string(evt->data_len));
            break;
        case HTTP_EVENT_ON_FINISH:
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP Client finished");
            break;
        case HTTP_EVENT_DISCONNECTED:
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP Client disconnected");
            break;
        default:
            break;
    }
    return ESP_OK;
}

void TelegramInit(std::string _token, std::string _chat_id)
{
    _botToken = _token;
    _chatId = _chat_id;
    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Telegram Bot initialized");
}

bool TelegramSendMessage(std::string message)
{
    if (_botToken.empty() || _chatId.empty()) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Bot token or chat ID not set");
        return false;
    }

    // Limit message length to prevent memory issues
    if (message.length() > 4000) {
        message = message.substr(0, 4000) + "...";
    }

    std::string url = "https://api.telegram.org/bot" + _botToken + "/sendMessage";
    
    // Create JSON payload
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to create JSON object");
        return false;
    }
    
    cJSON_AddStringToObject(json, "chat_id", _chatId.c_str());
    cJSON_AddStringToObject(json, "text", message.c_str());
    cJSON_AddStringToObject(json, "parse_mode", "HTML");
    
    char *jsonString = cJSON_PrintUnformatted(json);
    if (!jsonString) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to create JSON string");
        cJSON_Delete(json);
        return false;
    }
    
    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Sending message to Telegram");
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "JSON payload length: " + std::to_string(strlen(jsonString)));

    char response_buffer[512] = {0};  // Reduced buffer size
    esp_http_client_config_t http_config = {};
    http_config.url = url.c_str();
    http_config.user_agent = "ESP32 AI-on-the-edge Device";
    http_config.method = HTTP_METHOD_POST;
    http_config.event_handler = http_event_handler;
    http_config.buffer_size = 512;  // Reduced buffer size
    http_config.user_data = response_buffer;
    http_config.timeout_ms = 10000;  // 10 second timeout

    esp_http_client_handle_t http_client = esp_http_client_init(&http_config);
    if (!http_client) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to initialize HTTP client");
        cJSON_Delete(json);
        free(jsonString);
        return false;
    }

    esp_http_client_set_header(http_client, "Content-Type", "application/json");
    esp_http_client_set_post_field(http_client, jsonString, strlen(jsonString));

    esp_err_t err = esp_http_client_perform(http_client);
    bool success = false;

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(http_client);
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP status code: " + std::to_string(status_code));
        
        if (status_code == 200) {
            LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Message sent successfully to Telegram");
            success = true;
        } else {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Telegram API returned status: " + std::to_string(status_code));
        }
    } else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "HTTP request failed: " + std::string(esp_err_to_name(err)));
    }

    esp_http_client_cleanup(http_client);
    cJSON_Delete(json);
    free(jsonString);
    
    return success;
}

bool TelegramSendPhoto(const uint8_t* imageData, size_t imageSize, std::string caption)
{
    // Disable photo sending for now to prevent memory issues
    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Photo sending disabled for stability");
    return true;
}

#endif //ENABLE_TELEGRAM