#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <list>
#include <limits>
#include <mutex>
#include <string>
#include <vector>

#include "FactionTemplate.h"

class VMapReader final {
public:
    struct Liquid { bool found = false; std::uint32_t type = 0; float surface = 0, ground = 0; };

    static Liquid At(unsigned mapId, int tileX, int tileY, float x, float y, float z) {
        static std::mutex mutex;
        static std::list<Tile> tiles;
        static std::list<Model> models;
        static const auto directory = std::filesystem::path(FactionTemplateDBC::GetModulePath()) / "vmaps";
        std::lock_guard<std::mutex> lock(mutex);
        const std::uint64_t key = (std::uint64_t(mapId) << 12) | (std::uint64_t(tileX) << 6) | tileY;
        auto tile = std::find_if(tiles.begin(), tiles.end(), [key](const Tile& t) { return t.key == key; });
        if (tile == tiles.end()) {
            Tile t;
            t.key = key;
            char name[48];
            std::snprintf(name, sizeof(name), "%03u.vmtree", mapId);
            std::ifstream file(directory / name, std::ios::binary);
            char magic[8]{}, chunk[4]{};
            std::uint8_t tiled = 0;
            if (Read(file, magic) && std::memcmp(magic, "VMAP_7.0", 8) == 0 &&
                Read(file, tiled) && Read(file, chunk) && std::memcmp(chunk, "NODE", 4) == 0 &&
                ReadBih(file, nullptr) && Read(file, chunk) && std::memcmp(chunk, "GOBJ", 4) == 0) {
                if (tiled) {
                    std::snprintf(name, sizeof(name), "%03u_%02d_%02d.vmtile", mapId, tileY, tileX);
                    std::ifstream tileFile(directory / name, std::ios::binary);
                    std::uint32_t count = 0;
                    if (Read(tileFile, magic) && std::memcmp(magic, "VMAP_7.0", 8) == 0 &&
                        Read(tileFile, count) && count < 100000) {
                        for (std::uint32_t i = 0; i < count; ++i) {
                            Instance spawn;
                            std::uint32_t reference;
                            if (!ReadSpawn(tileFile, spawn) || !Read(tileFile, reference)) break;
                            if (!(spawn.flags & 1)) t.instances.push_back(std::move(spawn));
                        }
                    }
                } else {
                    while (file.peek() != EOF) {
                        Instance spawn;
                        std::uint32_t reference;
                        if (!ReadSpawn(file, spawn) || !Read(file, reference)) break;
                        if (!(spawn.flags & 1)) t.instances.push_back(std::move(spawn));
                    }
                }
            }
            tiles.push_front(std::move(t));
            if (tiles.size() > 24) tiles.pop_back();
            tile = tiles.begin();
        } else if (tile != tiles.begin()) {
            tiles.splice(tiles.begin(), tiles, tile);
            tile = tiles.begin();
        }

        const Vec pos{ 32.0f * 533.33333333f - x, 32.0f * 533.33333333f - y, z };
        Liquid best;
        float nearestGround = -std::numeric_limits<float>::infinity();
        for (const Instance& instance : tile->instances) {
            if (instance.hasBounds && !Inside(instance.low, instance.high, pos)) continue;
            auto model = std::find_if(models.begin(), models.end(), [&instance](const Model& m) { return m.name == instance.name; });
            if (model == models.end()) {
                Model m;
                m.name = instance.name;
                std::ifstream file(directory / (m.name + ".vmo"), std::ios::binary);
                char magic[8]{}, chunk[4]{};
                std::uint32_t size = 0, root = 0, count = 0;
                if (Read(file, magic) && std::memcmp(magic, "VMAP_7.0", 8) == 0 &&
                    Read(file, chunk) && std::memcmp(chunk, "WMOD", 4) == 0 &&
                    Read(file, size) && Read(file, root) &&
                    Read(file, chunk) && std::memcmp(chunk, "GMOD", 4) == 0 &&
                    Read(file, count) && count < 10000) {
                    m.groups.reserve(count);
                    bool valid = true;
                    for (std::uint32_t i = 0; i < count && valid; ++i) {
                        Group g;
                        std::uint32_t flags = 0, groupId = 0, chunkSize = 0, vertices = 0, triangles = 0;
                        valid = Read(file, g.low) && Read(file, g.high) && Read(file, flags) &&
                            Read(file, groupId) && Read(file, chunk) && std::memcmp(chunk, "VERT", 4) == 0 &&
                            Read(file, chunkSize) && Read(file, vertices) && vertices < 2000000;
                        if (!valid) break;
                        if (!vertices) { m.groups.emplace_back(std::move(g)); continue; }
                        g.vertices.resize(vertices);
                        valid = Read(file, g.vertices.data(), vertices) &&
                            Read(file, chunk) && std::memcmp(chunk, "TRIM", 4) == 0 &&
                            Read(file, chunkSize) && Read(file, triangles) && triangles < 4000000;
                        if (!valid) break;
                        g.triangles.resize(triangles);
                        valid = Read(file, g.triangles.data(), triangles) &&
                            Read(file, chunk) && std::memcmp(chunk, "MBIH", 4) == 0 &&
                            ReadBih(file, &g.bih) && Read(file, chunk) && std::memcmp(chunk, "LIQU", 4) == 0 &&
                            Read(file, chunkSize);
                        if (!valid) break;
                        if (chunkSize) {
                            valid = Read(file, g.liquid.width) && Read(file, g.liquid.height) &&
                                Read(file, g.liquid.corner) && Read(file, g.liquid.type) &&
                                g.liquid.width > 0 && g.liquid.height > 0 &&
                                g.liquid.width <= 1024 && g.liquid.height <= 1024;
                            if (!valid) break;
                            const std::size_t corners = std::size_t(g.liquid.width + 1) * (g.liquid.height + 1);
                            const std::size_t cells = std::size_t(g.liquid.width) * g.liquid.height;
                            g.liquid.heights.resize(corners);
                            g.liquid.flags.resize(cells);
                            valid = Read(file, g.liquid.heights.data(), corners) &&
                                Read(file, g.liquid.flags.data(), cells);
                            if (valid) m.hasLiquid = true;
                        }
                        m.groups.emplace_back(std::move(g));
                    }
                    if (!valid) { m.groups.clear(); m.hasLiquid = false; }
                }
                if (!m.hasLiquid) m.groups.clear();
                models.push_front(std::move(m));
                if (models.size() > 12) models.pop_back();
                model = models.begin();
            } else if (model != models.begin()) {
                models.splice(models.begin(), models, model);
                model = models.begin();
            }
            if (!model->hasLiquid || instance.scale <= 0) continue;
            const Vec delta{ pos.x - instance.pos.x, pos.y - instance.pos.y, pos.z - instance.pos.z };
            const Vec local = Rotate(instance, delta) / instance.scale;
            const Vec down = Rotate(instance, { 0, 0, -1 });
            const Vec ray{ local.x - 0.1f * down.x, local.y - 0.1f * down.y, local.z - 0.1f * down.z };
            for (const Group& group : model->groups) {
                if (group.vertices.empty() || !Inside(group.low, group.high, local)) continue;
                const float distance = GroundDistance(group, ray, down);
                if (!std::isfinite(distance)) continue;
                const float ground = pos.z - (distance - 0.1f) * instance.scale;
                if (ground <= nearestGround) continue;
                nearestGround = ground;
                best = {};
                const auto& liquid = group.liquid;
                if (liquid.heights.empty()) continue;
                constexpr float cellSize = 533.333f / 128.0f;
                const float fx = (local.x - liquid.corner.x) / cellSize;
                const float fy = (local.y - liquid.corner.y) / cellSize;
                if (fx < 0 || fy < 0 || fx >= liquid.width || fy >= liquid.height) continue;
                const unsigned ix = static_cast<unsigned>(fx), iy = static_cast<unsigned>(fy);
                if ((liquid.flags[ix + iy * liquid.width] & 0x0f) == 0x0f) continue;
                const float dx = fx - ix, dy = fy - iy;
                const unsigned row = liquid.width + 1, base = ix + iy * row;
                const float* h = liquid.heights.data();
                const float height = dx > dy ? h[base] + dx * (h[base + 1] - h[base]) +
                    dy * (h[base + 1 + row] - h[base + 1]) :
                    h[base] + dx * (h[base + 1 + row] - h[base + row]) +
                    dy * (h[base + row] - h[base]);
                const float surface = pos.z + instance.scale * (height - local.z);
                if (std::isfinite(surface) && surface > ground)
                    best = { true, liquid.type, surface, ground };
            }
        }
        return best;
    }

private:
    struct Vec {
        float x = 0, y = 0, z = 0;
        Vec operator/(float v) const { return { x / v, y / v, z / v }; }
    };
    struct Bih {
        Vec low, high;
        std::vector<std::uint32_t> tree, objects;
    };
    struct WmoLiquid {
        std::uint32_t width = 0, height = 0, type = 0;
        Vec corner;
        std::vector<float> heights;
        std::vector<std::uint8_t> flags;
    };
    struct Group {
        Vec low, high;
        std::vector<Vec> vertices;
        std::vector<std::array<std::uint32_t, 3>> triangles;
        Bih bih;
        WmoLiquid liquid;
    };
    struct Model {
        std::string name;
        bool hasLiquid = false;
        std::vector<Group> groups;
    };
    struct Instance {
        std::uint32_t flags = 0;
        std::uint16_t adt = 0;
        std::uint32_t id = 0;
        Vec pos, rot, low, high;
        float scale = 1;
        float inverse[9]{};
        bool hasBounds = false;
        std::string name;
    };
    struct Tile {
        std::uint64_t key = 0;
        std::vector<Instance> instances;
    };

    template <typename T> static bool Read(std::ifstream& file, T& value) {
        return bool(file.read(reinterpret_cast<char*>(&value), sizeof(value)));
    }
    template <typename T> static bool Read(std::ifstream& file, T* values, std::size_t count) {
        return count <= 16000000 && bool(file.read(reinterpret_cast<char*>(values), sizeof(T) * count));
    }
    static bool Inside(const Vec& low, const Vec& high, const Vec& p) {
        return p.x >= low.x && p.x <= high.x && p.y >= low.y && p.y <= high.y &&
            p.z >= low.z && p.z <= high.z;
    }
    static Vec Rotate(const Instance& instance, const Vec& v) {
        const float* m = instance.inverse;
        return { m[0] * v.x + m[1] * v.y + m[2] * v.z,
                 m[3] * v.x + m[4] * v.y + m[5] * v.z,
                 m[6] * v.x + m[7] * v.y + m[8] * v.z };
    }
    static bool ReadSpawn(std::ifstream& file, Instance& s) {
        std::uint32_t length = 0;
        if (!Read(file, s.flags) || !Read(file, s.adt) || !Read(file, s.id) ||
            !Read(file, s.pos) || !Read(file, s.rot) || !Read(file, s.scale)) return false;
        s.hasBounds = (s.flags & 4) != 0;
        if (s.hasBounds && (!Read(file, s.low) || !Read(file, s.high))) return false;
        if (!Read(file, length) || length > 500) return false;
        s.name.resize(length);
        if (length && !file.read(&s.name[0], length)) return false;
        if (!(s.scale > 0.0f) || !std::isfinite(s.scale)) return false;
        constexpr float rad = 3.14159265358979323846f / 180.0f;
        const float a = s.rot.y * rad, b = s.rot.x * rad, c = s.rot.z * rad;
        const float ca = std::cos(a), sa = std::sin(a), cb = std::cos(b), sb = std::sin(b);
        const float cc = std::cos(c), sc = std::sin(c);
        const float m[] = { ca * cb, sa * cb, -sb,
            ca * sb * sc - sa * cc, sa * sb * sc + ca * cc, cb * sc,
            ca * sb * cc + sa * sc, sa * sb * cc - ca * sc, cb * cc };
        std::copy(m, m + 9, s.inverse);
        return true;
    }
    static bool ReadBih(std::ifstream& file, Bih* bih) {
        Vec low, high;
        std::uint32_t count = 0;
        if (!Read(file, low) || !Read(file, high) || !Read(file, count) || count > 16000000) return false;
        if (bih) {
            bih->low = low;
            bih->high = high;
            bih->tree.resize(count);
            if (!Read(file, bih->tree.data(), count)) return false;
        } else file.seekg(std::streamoff(count) * 4, std::ios::cur);
        if (!Read(file, count) || count > 16000000) return false;
        if (bih) {
            bih->objects.resize(count);
            return Read(file, bih->objects.data(), count);
        }
        file.seekg(std::streamoff(count) * 4, std::ios::cur);
        return bool(file);
    }
    static float GroundDistance(const Group& group, const Vec& origin, const Vec& dir) {
        const auto& bih = group.bih;
        if (bih.tree.empty()) return std::numeric_limits<float>::infinity();
        float tMin = 0, tMax = std::numeric_limits<float>::infinity();
        for (int i = 0; i < 3; ++i) {
            const float o = (&origin.x)[i], d = (&dir.x)[i];
            const float lo = (&bih.low.x)[i], hi = (&bih.high.x)[i];
            if (std::abs(d) < 1.0e-8f) { if (o < lo || o > hi) return std::numeric_limits<float>::infinity(); }
            else {
                float a = (lo - o) / d, b = (hi - o) / d;
                if (a > b) std::swap(a, b);
                tMin = (std::max)(tMin, a);
                tMax = (std::min)(tMax, b);
                if (tMin > tMax) return std::numeric_limits<float>::infinity();
            }
        }
        const auto& tree = bih.tree;
        const auto& objects = bih.objects;
        struct Node { std::uint32_t index; float tMin, tMax; };
        std::array<Node, 128> stack{};
        std::size_t top = 0;
        std::uint32_t node = 0;
        float best = std::numeric_limits<float>::infinity();
        while (true) {
            if (node >= tree.size() || tree.size() - node < 3) break;
            const std::uint32_t code = tree[node], axis = code >> 30, offset = code & 0x1fffffff;
            const bool bvh2 = (code & 0x20000000) != 0;
            if (axis == 3 && !bvh2) {
                const std::uint32_t count = tree[node + 1];
                if (offset <= objects.size() && count <= objects.size() - offset) {
                    for (std::uint32_t i = 0; i < count; ++i) {
                        const std::uint32_t index = objects[offset + i];
                        if (index >= group.triangles.size()) continue;
                        const auto& tri = group.triangles[index];
                        if (tri[0] >= group.vertices.size() || tri[1] >= group.vertices.size() || tri[2] >= group.vertices.size()) continue;
                        const Vec& a = group.vertices[tri[0]];
                        const Vec& b = group.vertices[tri[1]];
                        const Vec& c = group.vertices[tri[2]];
                        const Vec e1{ b.x - a.x, b.y - a.y, b.z - a.z };
                        const Vec e2{ c.x - a.x, c.y - a.y, c.z - a.z };
                        const Vec p{ dir.y * e2.z - dir.z * e2.y, dir.z * e2.x - dir.x * e2.z, dir.x * e2.y - dir.y * e2.x };
                        const float det = e1.x * p.x + e1.y * p.y + e1.z * p.z;
                        if (std::abs(det) < 1.0e-7f) continue;
                        const Vec t{ origin.x - a.x, origin.y - a.y, origin.z - a.z };
                        const float u = (t.x * p.x + t.y * p.y + t.z * p.z) / det;
                        if (u < 0 || u > 1) continue;
                        const Vec q{ t.y * e1.z - t.z * e1.y, t.z * e1.x - t.x * e1.z, t.x * e1.y - t.y * e1.x };
                        const float v = (dir.x * q.x + dir.y * q.y + dir.z * q.z) / det;
                        if (v < 0 || u + v > 1) continue;
                        const float distance = (e2.x * q.x + e2.y * q.y + e2.z * q.z) / det;
                        if (distance >= 0 && distance < best) best = distance;
                    }
                }
            } else if (axis < 3 && offset + (bvh2 ? 2u : 5u) < tree.size()) {
                float clip0, clip1;
                std::memcpy(&clip0, &tree[node + 1], 4);
                std::memcpy(&clip1, &tree[node + 2], 4);
                const float o = (&origin.x)[axis], d = (&dir.x)[axis];
                if (bvh2) {
                    if (std::abs(d) < 1.0e-8f) {
                        if (o >= clip0 && o <= clip1) { node = offset; continue; }
                    } else {
                        float t0 = (clip0 - o) / d, t1 = (clip1 - o) / d;
                        if (t0 > t1) std::swap(t0, t1);
                        tMin = (std::max)(tMin, t0);
                        tMax = (std::min)(tMax, t1);
                        if (tMin <= tMax && tMin <= best) { node = offset; continue; }
                    }
                } else {
                    const std::uint32_t left = offset, right = offset + 3;
                    float firstNear = tMin, firstFar = tMax, secondNear = tMin, secondFar = tMax;
                    const bool reverse = d < 0;
                    const float firstClip = reverse ? clip1 : clip0;
                    const float secondClip = reverse ? clip0 : clip1;
                    if (std::abs(d) < 1.0e-8f) {
                        if (o > clip0) firstNear = best + 1;
                        if (o < clip1) secondNear = best + 1;
                    } else {
                        firstFar = (std::min)(firstFar, (firstClip - o) / d);
                        secondNear = (std::max)(secondNear, (secondClip - o) / d);
                    }
                    if (secondNear <= secondFar && secondNear <= best && top < stack.size())
                        stack[top++] = { reverse ? left : right, secondNear, secondFar };
                    if (firstNear <= firstFar && firstNear <= best) {
                        node = reverse ? right : left;
                        tMin = firstNear;
                        tMax = firstFar;
                        continue;
                    }
                }
            }
            if (!top) break;
            const Node next = stack[--top];
            node = next.index;
            tMin = next.tMin;
            tMax = next.tMax;
        }
        return best;
    }
};
