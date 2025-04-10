// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
//
// Created by boris on 10.04.2025.
//

#pragma once
#include <cstdint>


class YDec
{
    uint8_t key[16] = {};
    uint32_t blockNumber = 0;

public:
    YDec();
    void init(const char* key);
    void  decrypt(void* data, uint64_t size);
};
