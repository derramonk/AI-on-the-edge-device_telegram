#ifdef ENABLE_TELEGRAM

#pragma once

#ifndef CLASSFLOWTELEGRAM_H
#define CLASSFLOWTELEGRAM_H

#include "ClassFlow.h"
#include "ClassFlowPostProcessing.h"
#include "ClassFlowAlignment.h"
#include <string>

class ClassFlowTelegram : public ClassFlow
{
protected:
    std::string botToken, chatId;
    ClassFlowPostProcessing* flowpostprocessing;
    ClassFlowAlignment* flowAlignment;

    bool TelegramEnable;
    int TelegramUploadImg;
    bool TelegramOnError;
    
    void SetInitialParameter(void);

public:
    ClassFlowTelegram();
    ClassFlowTelegram(std::vector<ClassFlow*>* lfc);
    ClassFlowTelegram(std::vector<ClassFlow*>* lfc, ClassFlow *_prev);

    bool ReadParameter(FILE* pfile, string& aktparamgraph);
    bool doFlow(string time);
    string name(){return "ClassFlowTelegram";};
};

#endif //CLASSFLOWTELEGRAM_H
#endif //ENABLE_TELEGRAM