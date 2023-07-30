//
// Created by boris on 29.07.23.
//
#pragma once
#include <gst/gst.h>
#include <flutter_linux/flutter_linux.h>

class AudioPlayer
{
    FlEventChannel* _eventChannel;
    GstElement* _playbin = nullptr;
    GstBus* _bus = nullptr;
    guint _refreshTimer{};
    bool _isLive = false;
//    gint _bufferingLevel{};
    gint64 _downloadProgress{};
    gboolean _seekEnabled = false;

    static gboolean _onBusMessage(GstBus *bus, GstMessage *message, AudioPlayer *data);
    static gboolean _onRefreshTick(AudioPlayer *data);

public:
    explicit AudioPlayer(FlEventChannel* eventChannel);
    ~AudioPlayer();

    void play();
    void pause();
    void setVolume(double value);
    void setUrl(const char* urlString);
    void seek(gint64 position);
    gint64 duration();
};
