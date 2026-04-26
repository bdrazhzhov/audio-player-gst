//
// Created by boris on 01.08.23.
//
#include "flutter_event_sender.h"

struct SendTaskData
{
    FlEventChannel* channel;
    std::string eventName;
    std::function<void(const FlValue_autoptr&)> dataFunc;
};

FlutterEventSender::FlutterEventSender(FlEventChannel* eventChannel, GMainContext* platformContext)
    : _eventChannel(eventChannel)
{
    if(platformContext)
    {
        _platformContext = g_main_context_ref(platformContext);
    }
    else
    {
        _platformContext = g_main_context_ref(g_main_context_default());
    }
}

FlutterEventSender::~FlutterEventSender()
{
    g_main_context_unref(_platformContext);
}

void FlutterEventSender::_send(const char* eventName, const std::function<void(const FlValue_autoptr&)>& dataFunc)
{
    if(!_eventChannel) return;

    auto* taskData = new SendTaskData {
        static_cast<FlEventChannel*>(g_object_ref(_eventChannel)),
        eventName,
        dataFunc
    };

    g_main_context_invoke_full(_platformContext,
                               G_PRIORITY_DEFAULT,
                               _sendTask,
                               taskData,
                               _destroySendTask);
}

gboolean FlutterEventSender::_sendTask(gpointer userData)
{
    const auto* taskData = static_cast<SendTaskData*>(userData);
    g_autoptr(FlValue) map = fl_value_new_map();
    fl_value_set_string(map, "event", fl_value_new_string(taskData->eventName.c_str()));
    taskData->dataFunc(map);
    fl_event_channel_send(taskData->channel, map, nullptr, nullptr);

    return G_SOURCE_REMOVE;
}

void FlutterEventSender::_destroySendTask(gpointer userData)
{
    auto* taskData = static_cast<SendTaskData*>(userData);
    g_object_unref(taskData->channel);
    delete taskData;
}

void FlutterEventSender::operator()(const char* eventName, const char* data)
{
    _send(eventName, [data](const FlValue_autoptr& map){
        fl_value_set_string(map, "value", fl_value_new_string(data));
    });
}

void FlutterEventSender::operator()(const char* eventName, const std::string& data)
{
  (*this)(eventName, data.c_str());
}

void FlutterEventSender::operator()(const char* eventName, bool value)
{
    _send(eventName, [value](const FlValue_autoptr& map){
        fl_value_set_string(map, "value", fl_value_new_bool(value));
    });
}

void FlutterEventSender::operator()(const char* eventName, int64_t value)
{
    _send(eventName, [value](const FlValue_autoptr& map){
        fl_value_set_string(map, "value", fl_value_new_int(value));
    });
}

void FlutterEventSender::operator()(const char* eventName, double value)
{
    _send(eventName, [value](const FlValue_autoptr& map){
        fl_value_set_string(map, "value", fl_value_new_float(value));
    });
}

