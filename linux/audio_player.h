//
// Created by boris on 29.07.23.
//
#pragma once
#include <gst/gst.h>
#include <flutter_linux/flutter_linux.h>
#include <memory>
#include "flutter_event_sender.h"

class AudioPlayer
{
    std::unique_ptr<FlutterEventSender> _eventSender;
    GstElement* _playbin = nullptr;
    GstBus* _bus = nullptr;
    guint _refreshTimer{};
    bool _isBuffered = false;
    gint _bufferingLevel{};
    bool _isLive = false;
    gint64 _downloadProgress{};
    gboolean _seekEnabled = false;
    gint64 _position{};
    gdouble _rate = 1.0;
    gdouble _volume = 1.0;
    bool _isUrlSet = false;
    bool _isVolumeAboutToSet = false;

    static gboolean _onBusMessage(GstBus *bus, GstMessage *message, AudioPlayer *data);
    static gboolean _onRefreshTick(AudioPlayer *data);
    static void _onVolumeChanged(GstElement* volume, GParamSpec* pspec, AudioPlayer* user_data);
    void _seek(gint64 position, gdouble rate);

public:
    explicit AudioPlayer(FlEventChannel* eventChannel);
    ~AudioPlayer();

    void play();
    void pause();
    void setVolume(double value);
    void setUrl(const char* urlString);
    void seek(gint64 position);
    void setRate(double rate);
    gint64 duration();
};
