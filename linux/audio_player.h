//
// Created by boris on 29.07.23.
//

#pragma once
#include <gst/gst.h>
#include <flutter_linux/flutter_linux.h>
#include "flutter_event_sender.h"

#include "app-src.h"
#include "y-downloader.h"

class AudioPlayer
{
    FlutterEventSender _sendEvent;
    GstElement* _playbin = nullptr;
    GstElement* _scaleTempo = nullptr;
    GstBus* _bus = nullptr;
    guint _refreshTimer{};
    bool _isDownloaded = false;
    gboolean _seekEnabled = false;
    gint64 _position{};
    gdouble _rate = 1.0;
    gdouble _volume = 1.0;
    bool _isUrlSet = false;
    bool _isVolumeAboutToSet = false;
    AppSrc _appSrc;

    static gboolean _onBusMessage(GstBus *bus, GstMessage *message, AudioPlayer *self);
    static gboolean _onRefreshTick(AudioPlayer *self);
    static void _onVolumeChanged(GstElement* volumeElement, GParamSpec* pspec, AudioPlayer* self);
    void _seek(gint64 position, gdouble rate);
    static void sourceSetup(GstElement* pipeline, GstElement* source, AudioPlayer* self);

public:
    explicit AudioPlayer(FlEventChannel* eventChannel);
    ~AudioPlayer();

    void play();
    void pause() const;
    void setVolume(double value);
    void setUrl(const char* urlString, const char* encryptionKey = nullptr);
    void seek(gint64 position);
    void setRate(double rate);
    [[nodiscard]] gint64 duration() const;
};


