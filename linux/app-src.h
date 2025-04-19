// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
//
// Created by boris on 10.04.2025.
//

#pragma once
#include <gst/app/gstappsrc.h>
#include "y-downloader.h"

class AppSrc
{
    YDownloader downloader;
    uint64_t offset = 0;
    GstAppSrc* source = nullptr;

public:
    AppSrc();
    void init(const char* url, const char* key);
    void sourceSetup(GstElement* source);
    [[nodiscard]] float downloadProgress() const { return downloader.progress(); }

private:
    static void appSourcePush(GstElement*, guint unusedSize, AppSrc* self);
    static gboolean appSourceSeek(GstElement*, guint64 offset, AppSrc* self);
};
