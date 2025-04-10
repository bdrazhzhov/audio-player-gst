// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
//
// Created by boris on 10.04.2025.
//

#include "y-dec.h"

#include <cstdint>
#include <gcrypt.h>
#include <stdexcept>

static void numberToUint8Counter(uint64_t num, uint8_t counter[16])
{
    for(int i = 0; i < 16; i++)
    {
        counter[15 - i] = static_cast<uint8_t>(num & 0xFF);
        num >>= 8;
    }
}

void hex32ToBytes(const char* hexString, uint8_t out[16])
{
    static const auto hexCharToValue = [](const char c)
    {
        if('0' <= c && c <= '9') return c - '0';
        if('a' <= c && c <= 'f') return c - 'a' + 10;
        if('A' <= c && c <= 'F') return c - 'A' + 10;

        return 255;
    };

    for(int i = 0; i < 16; ++i)
    {
        const uint8_t high = hexCharToValue(hexString[i * 2]);
        const uint8_t low = hexCharToValue(hexString[i * 2 + 1]);
        out[i] = (high << 4) | low;
    }
}

YDec::YDec()
{
    gcry_check_version(nullptr);
}

void YDec::init(const char* key)
{
    hex32ToBytes(key, this->key);
    blockNumber = 0;
}

void YDec::decrypt(void* data, const uint64_t size)
{
    uint8_t iv[16] = {};
    numberToUint8Counter(blockNumber, iv);

    gcry_cipher_hd_t handle;
    gcry_cipher_open(&handle, GCRY_CIPHER_AES128, GCRY_CIPHER_MODE_CTR, 0);
    gcry_cipher_setkey(handle, key, 16);
    gcry_cipher_setctr(handle, iv, 16);
    gcry_cipher_decrypt(handle, data, size, data, size);
    gcry_cipher_close(handle);

    blockNumber += size / 16;
}
