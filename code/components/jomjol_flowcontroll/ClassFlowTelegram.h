#ifdef ENABLE_TELEGRAM

#pragma once

#ifndef CLASSFLOWTELEGRAM_H
#define CLASSFLOWTELEGRAM_H

#include "ClassFlow.h"
#include "ClassFlowPostProcessing.h"
#include "ClassFlowAlignment.h"
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class ClassFlowTelegram : public ClassFlow
{
protected:
    std::string botToken, chatId;
    ClassFlowPostProcessing* flowpostprocessing;
    ClassFlowAlignment* flowAlignment;

    bool TelegramEnable;
    int TelegramUploadImg;
    bool TelegramOnError;
    bool TelegramBotCommands;
    
    void SetInitialParameter(void);

    // Static members for task handling
    static TaskHandle_t telegramTaskHandle;
    static ClassFlowTelegram* instance;

    // Task functions
    static void telegramTask(void* parameter);
    
    // Task management methods
    void startTelegramTask();
    void stopTelegramTask();

public:
    ClassFlowTelegram();
    ClassFlowTelegram(std::vector<ClassFlow*>* lfc);
    ClassFlowTelegram(std::vector<ClassFlow*>* lfc, ClassFlow *_prev);
    ~ClassFlowTelegram();

    bool ReadParameter(FILE* pfile, string& aktparamgraph);
    bool doFlow(string time);
    string name(){return "ClassFlowTelegram";};
};

#endif //CLASSFLOWTELEGRAM_H
#endif //ENABLE_TELEGRAM