#pragma once

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <list>
#include <mutex>
#include <string>
#include <vector>

#include "FactionTemplate.h"
#include "VMapReader.h"

class WaterMap final {
public:
    enum class Kind { Unknown, Dry, Shallow, Swimmable, Hazard };
    enum class Level { Unknown, Above, Surface, In, Under };
    struct Sample {
        Kind kind = Kind::Unknown;
        float surface = 0.0f;
        float ground = 0.0f;
        bool Known() const { return kind != Kind::Unknown; }
        bool CanSwim() const { return kind == Kind::Swimmable; }
        float Depth() const { return surface - ground; }
        Level RelativeTo(float z, float collisionHeight = 2.03128f) const {
            if (!CanSwim() || z < ground - 2.0f) return Level::Unknown;
            const float delta = surface - z;
            if (delta > collisionHeight) return Level::Under;
            if (delta > 0.0f) return Level::In;
            if (delta > -1.0f) return Level::Surface;
            return Level::Above;
        }
    };

    static Sample At(unsigned mapId, float x, float y, float z) {
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return {};
        constexpr float GridSize = 533.333333f;
        const int gridX = static_cast<int>(32.0f - x / GridSize);
        const int gridY = static_cast<int>(32.0f - y / GridSize);
        if (gridX < 0 || gridX > 63 || gridY < 0 || gridY > 63) return {};
        const auto vmap = VMapReader::At(mapId, gridX, gridY, x, y, z);
        if (vmap.found) {
            if (vmap.type == 3 || vmap.type == 4 || vmap.type == 21)
                return { Kind::Hazard, vmap.surface, vmap.ground };
            if (vmap.type == 1 || vmap.type == 2 || vmap.type == 41 || vmap.type == 61)
                return { vmap.surface - vmap.ground < 1.6f ? Kind::Shallow : Kind::Swimmable,
                    vmap.surface, vmap.ground };
        }
        const std::uint64_t key = (std::uint64_t(mapId) << 12) |
            (std::uint64_t(gridX) << 6) | std::uint64_t(gridY);
        static std::mutex mutex;
        static std::list<Tile> tiles;
        std::lock_guard<std::mutex> lock(mutex);
        auto tileIt = std::find_if(tiles.begin(), tiles.end(),
            [key](const Tile& tile) { return tile.key == key; });
        if (tileIt == tiles.end()) {
            Tile tile;
            tile.key = key;
            char name[32];
            std::snprintf(name, sizeof(name), "%03u%02d%02d.map", mapId, gridX, gridY);
            static const auto mapsDirectory =
                std::filesystem::path(FactionTemplateDBC::GetModulePath()) / "maps";
            std::ifstream file(mapsDirectory / name, std::ios::binary);
            if (file) {
                struct FileHeader {
                    char magic[4], version[4];
                    std::uint32_t areaOffset, areaSize, heightOffset, heightSize;
                    std::uint32_t liquidOffset, liquidSize, holesOffset, holesSize;
                } header{};
                static_assert(sizeof(FileHeader) == 40);
                file.read(reinterpret_cast<char*>(&header), sizeof(header));
                if (file && std::memcmp(header.magic, "MAPS", 4) == 0 &&
                    (std::memcmp(header.version, "z1.4", 4) == 0 ||
                     std::memcmp(header.version, "s1.4", 4) == 0) &&
                    header.heightOffset >= sizeof(header) && header.heightOffset < 4'000'000 &&
                    header.heightSize >= 16 && header.heightSize < 250'000 &&
                    ((header.liquidOffset == 0 && header.liquidSize == 0) ||
                     (header.liquidOffset >= sizeof(header) && header.liquidOffset < 4'000'000 &&
                      header.liquidSize >= 16 && header.liquidSize < 200'000))) {
                    struct HeightHeader {
                        char magic[4]; std::uint32_t flags; float minimum, maximum;
                    } height{};
                    struct LiquidHeader {
                        char magic[4]; std::uint8_t flags, globalType;
                        std::uint16_t globalEntry;
                        std::uint8_t offsetX, offsetY, width, height;
                        float level;
                    } liquid{};
                    static_assert(sizeof(HeightHeader) == 16);
                    static_assert(sizeof(LiquidHeader) == 16);
                    file.seekg(header.heightOffset);
                    file.read(reinterpret_cast<char*>(&height), sizeof(height));
                    if (file && std::memcmp(height.magic, "MHGT", 4) == 0 &&
                        std::isfinite(height.minimum) && std::isfinite(height.maximum)) {
                        tile.baseHeight = height.minimum;
                        tile.compact = (height.flags & 6) != 0;
                        tile.multiplier = (height.flags & 2) ?
                            (height.maximum - height.minimum) / 65535.0f :
                            (height.flags & 4) ? (height.maximum - height.minimum) / 255.0f : 1.0f;
                        const int countV9 = 129 * 129, countV8 = 128 * 128;
                        const int valueSize = height.flags & 2 ? 2 : height.flags & 4 ? 1 : 4;
                        const std::uint32_t expected = 16 + ((height.flags & 1) ? 0 :
                            (countV9 + countV8) * valueSize);
                        if (expected <= header.heightSize) {
                            if (!(height.flags & 1)) {
                                tile.v9.resize(countV9);
                                tile.v8.resize(countV8);
                                std::vector<std::uint8_t> raw((countV9 + countV8) * valueSize);
                                file.read(reinterpret_cast<char*>(raw.data()), raw.size());
                                if (file) {
                                    for (std::size_t i = 0; i < tile.v9.size() + tile.v8.size(); ++i) {
                                        float v = 0.0f;
                                        if (valueSize == 4) std::memcpy(&v, raw.data() + i * 4, 4);
                                        else if (valueSize == 2) {
                                            std::uint16_t n;
                                            std::memcpy(&n, raw.data() + i * 2, 2);
                                            v = static_cast<float>(n);
                                        } else v = static_cast<float>(raw[i]);
                                        if (i < tile.v9.size()) tile.v9[i] = v;
                                        else tile.v8[i - tile.v9.size()] = v;
                                    }
                                }
                            }
                            if (file && header.liquidOffset == 0 && header.liquidSize == 0) tile.valid = true;
                            if (header.liquidOffset != 0) file.seekg(header.liquidOffset);
                            if (header.liquidOffset != 0) file.read(reinterpret_cast<char*>(&liquid), sizeof(liquid));
                            if (header.liquidOffset != 0 && file && std::memcmp(liquid.magic, "MLIQ", 4) == 0 &&
                                liquid.width <= 129 && liquid.height <= 129 &&
                                std::isfinite(liquid.level)) {
                                tile.globalType = liquid.globalType;
                                tile.hasHeightMap = !(liquid.flags & 2);
                                tile.surface = liquid.level;
                                tile.offsetX = liquid.offsetX;
                                tile.offsetY = liquid.offsetY;
                                tile.width = liquid.width;
                                tile.height = liquid.height;
                                const std::size_t total = 16 + (liquid.flags & 1 ? 0 : 768) +
                                    (liquid.flags & 2 ? 0 : std::size_t(liquid.width) * liquid.height * 4);
                                if (total <= header.liquidSize &&
                                    (!tile.hasHeightMap || (liquid.width && liquid.height))) {
                                    if (!(liquid.flags & 1)) {
                                        std::array<std::uint16_t, 256> entries{};
                                        file.read(reinterpret_cast<char*>(entries.data()), sizeof(entries));
                                        file.read(reinterpret_cast<char*>(tile.types.data()), tile.types.size());
                                        tile.hasTypes = true;
                                    }
                                    if (!(liquid.flags & 2)) {
                                        tile.liquidHeights.resize(std::size_t(liquid.width) * liquid.height);
                                        file.read(reinterpret_cast<char*>(tile.liquidHeights.data()),
                                            tile.liquidHeights.size() * sizeof(float));
                                    }
                                    tile.valid = bool(file);
                                }
                            }
                            if (tile.valid && header.holesOffset && header.holesSize == 512 &&
                                header.holesOffset < 4'000'000) {
                                file.seekg(header.holesOffset);
                                file.read(reinterpret_cast<char*>(tile.holes.data()), 512);
                                if (!file) tile.valid = false;
                                else tile.hasHoles = true;
                            }
                        }
                    }
                }
            }
            tiles.push_front(std::move(tile));
            if (tiles.size() > 8) tiles.pop_back();
            tileIt = tiles.begin();
        } else if (tileIt != tiles.begin()) {
            tiles.splice(tiles.begin(), tiles, tileIt);
            tileIt = tiles.begin();
        }
        const Tile& tile = *tileIt;
        if (!tile.valid) return {};
        const float gx = 128.0f * (32.0f - x / GridSize);
        const float gy = 128.0f * (32.0f - y / GridSize);
        const int ix = static_cast<int>(gx) & 127;
        const int iy = static_cast<int>(gy) & 127;
        if (tile.hasHoles) {
            const int cellX = ix / 8, cellY = iy / 8;
            const int bitX = (ix % 8) / 2, bitY = (iy % 8) / 2;
            constexpr std::uint16_t bitH[] = { 0x1111, 0x2222, 0x4444, 0x8888 };
            constexpr std::uint16_t bitV[] = { 0x000F, 0x00F0, 0x0F00, 0xF000 };
            if (tile.holes[cellX * 16 + cellY] & bitH[bitY] & bitV[bitX]) return {};
        }
        const std::uint8_t type = tile.hasTypes ? tile.types[(ix >> 3) * 16 + (iy >> 3)] : tile.globalType;
        if (!type) return { Kind::Dry };
        const int lx = ix - tile.offsetY;
        const int ly = iy - tile.offsetX;
        if (lx < 0 || ly < 0 || lx >= tile.height || ly >= tile.width)
            return { Kind::Dry };
        const float waterZ = tile.hasHeightMap ?
            tile.liquidHeights[lx * tile.width + ly] : tile.surface;
        if (!std::isfinite(waterZ)) return {};
        float groundZ = tile.baseHeight;
        if (!tile.v9.empty()) {
            const float tx = gx - static_cast<int>(gx);
            const float ty = gy - static_cast<int>(gy);
            const float h1 = tile.v9[ix * 129 + iy];
            const float h2 = tile.v9[(ix + 1) * 129 + iy];
            const float h3 = tile.v9[ix * 129 + iy + 1];
            const float h4 = tile.v9[(ix + 1) * 129 + iy + 1];
            const float h5 = 2.0f * tile.v8[ix * 128 + iy];
            float a, b, c;
            if (tx + ty < 1.0f) {
                if (tx > ty) { a = h2 - h1; b = h5 - h1 - h2; c = h1; }
                else { a = h5 - h1 - h3; b = h3 - h1; c = h1; }
            } else {
                if (tx > ty) { a = h2 + h4 - h5; b = h4 - h2; c = h5 - h4; }
                else { a = h4 - h3; b = h3 + h4 - h5; c = h5 - h4; }
            }
            groundZ = (a * tx + b * ty + c) * tile.multiplier +
                (tile.compact ? tile.baseHeight : 0.0f);
        }
        if (!std::isfinite(groundZ)) return {};
        if (waterZ <= groundZ) return { Kind::Dry };
        if (type & (0x01 | 0x04)) return { Kind::Hazard, waterZ, groundZ };
        if (!(type & (0x02 | 0x08))) return {};
        if (waterZ - groundZ < 1.6f) return { Kind::Shallow, waterZ, groundZ };
        return { Kind::Swimmable, waterZ, groundZ };
    }

private:
    struct Tile {
        std::uint64_t key = 0;
        bool valid = false, hasTypes = false, hasHeightMap = false, compact = false, hasHoles = false;
        std::array<std::uint16_t, 256> holes{};
        std::uint8_t globalType = 0, offsetX = 0, offsetY = 0, width = 0, height = 0;
        float baseHeight = 0.0f, multiplier = 1.0f, surface = 0.0f;
        std::array<std::uint8_t, 256> types{};
        std::vector<float> v9, v8, liquidHeights;
    };
};
