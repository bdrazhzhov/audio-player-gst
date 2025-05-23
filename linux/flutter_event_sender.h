//
// Created by boris on 01.08.23.
//
#pragma once
#include <flutter_linux/flutter_linux.h>
#include <string>
#include <functional>

class FlutterEventSender
{
    FlEventChannel* _eventChannel;

    void _send(const char* eventName, const std::function<void (const FlValue_autoptr&)>& dataFunc);

public:
    explicit FlutterEventSender(FlEventChannel* eventChannel) : _eventChannel(eventChannel) {}

    void operator()(const char* eventName, const char* data);
    void operator()(const char* eventName, const std::string& data);
    void operator()(const char* eventName, bool value);
    void operator()(const char* eventName, int64_t value);
    void operator()(const char* eventName, double value);
};
