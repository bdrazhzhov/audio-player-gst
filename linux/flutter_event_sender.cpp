//
// Created by boris on 01.08.23.
//
#include "flutter_event_sender.h"

void FlutterEventSender::_send(const char* eventName, const std::function<void(const FlValue_autoptr&)>& dataFunc)
{
    if(!_eventChannel) return;

    g_autoptr(FlValue) map = fl_value_new_map();
    fl_value_set_string(map, "event", fl_value_new_string(eventName));
    dataFunc(map);
    fl_event_channel_send(_eventChannel, map, nullptr, nullptr);
}

void FlutterEventSender::send(const char* eventName, const char* data)
{
    _send(eventName, [data](const FlValue_autoptr& map){
        fl_value_set_string(map, "value", fl_value_new_string(data));
    });
}

void FlutterEventSender::send(const char* eventName, const std::string& data)
{
    send(eventName, data.c_str());
}

void FlutterEventSender::send(const char* eventName, bool value)
{
    _send(eventName, [value](const FlValue_autoptr& map){
        fl_value_set_string(map, "value", fl_value_new_bool(value));
    });
}

void FlutterEventSender::send(const char* eventName, int64_t value)
{
    _send(eventName, [value](const FlValue_autoptr& map){
        fl_value_set_string(map, "value", fl_value_new_int(value));
    });
}

void FlutterEventSender::send(const char* eventName, double value)
{
    _send(eventName, [value](const FlValue_autoptr& map){
        fl_value_set_string(map, "value", fl_value_new_float(value));
    });
}


