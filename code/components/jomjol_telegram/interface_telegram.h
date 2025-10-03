#ifdef ENABLE_TELEGRAM

#pragma once
#ifndef INTERFACE_TELEGRAM_H
#define INTERFACE_TELEGRAM_H

#include <string>

void TelegramInit(std::string _botToken, std::string _chatId);
bool TelegramSendMessage(std::string message);
bool TelegramSendPhoto(const uint8_t* imageData, size_t imageSize, std::string caption = "");

#endif //INTERFACE_TELEGRAM_H
#endif //ENABLE_TELEGRAM