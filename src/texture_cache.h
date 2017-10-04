#pragma once

#include <map>
#include <stdint.h>
#include <vector>

class TextureCache
{
public:
    bool initialise();

private:
    bool load(const char* path);

    struct Texture
    {
        uint16_t width;
        uint16_t height;
        uint8_t channels;
        std::vector<uint8_t> pixels;
    };


    typedef std::map<const char*, Texture> Cache;
    Cache _textures;
};