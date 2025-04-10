// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
//
// Created by boris on 10.04.2025.
//

#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <thread>
#include <vector>
#include <libsoup/soup.h>

#include "y-dec.h"

struct ConnectionData
{
    SoupSession* session;
    SoupMessage* msg;
    GInputStream* stream;
};

class YDownloader
{
    std::vector<uint8_t> buffer;
    uint64_t dataOffset = 0;
    uint64_t _decryptOffset = 0;
    YDec ydec;
    std::unique_ptr<std::thread> worker;
    bool isRunning = false;
    std::function<void(uint64_t)> sizeCallback;
    bool needDecryption = false;

    bool establishConnection(const char* url, ConnectionData& connection) const;
    bool readDataChunk(const char* url, ConnectionData& connection);
    void processDownload(const char* url);
    void decrypt();

public:
    void start(const char* url, const char* key);
    void cancel();
    [[nodiscard]] uint8_t progress() const;
    [[nodiscard]] const uint8_t* data() const { return buffer.data(); }
    [[nodiscard]] uint64_t dataSize() const { return buffer.size(); }
    [[nodiscard]] uint64_t decryptOffset() const { return _decryptOffset; }
    void gotSizeCallback(const std::function<void(uint64_t)>& callback) { sizeCallback = callback; }
};
