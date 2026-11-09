#include "LibraryFormats.h"
#include <cstdlib>
#include <fstream>

namespace {
    constexpr uint32_t Magic(char a, char b, char c, char d) {
        return (uint32_t)(unsigned char)a | ((uint32_t)(unsigned char)b << 8)
            | ((uint32_t)(unsigned char)c << 16) | ((uint32_t)(unsigned char)d << 24);
    }
    constexpr uint32_t MESH_MAGIC = Magic('K', 'M', 'S', 'H');
    constexpr uint32_t TEXTURE_MAGIC = Magic('K', 'T', 'E', 'X');
    constexpr uint32_t MODEL_MAGIC = Magic('K', 'M', 'D', 'L');
    constexpr uint32_t FORMAT_VERSION = 1;
    constexpr uint32_t MAX_ELEMENTS = 1u << 30;   // límite de cordura al leer

    template<typename T>
    void Write(std::ofstream& f, const T& value) {
        f.write(reinterpret_cast<const char*>(&value), sizeof(T));
    }

    template<typename T>
    bool Read(std::ifstream& f, T& value) {
        f.read(reinterpret_cast<char*>(&value), sizeof(T));
        return static_cast<bool>(f);
    }

    void WriteString(std::ofstream& f, const std::string& s) {
        Write<uint32_t>(f, (uint32_t)s.size());
        f.write(s.data(), (std::streamsize)s.size());
    }

    bool ReadString(std::ifstream& f, std::string& s) {
        uint32_t size = 0;
        if (!Read(f, size) || size > 65536) return false;
        s.resize(size);
        f.read(s.data(), (std::streamsize)size);
        return static_cast<bool>(f);
    }

    template<typename T>
    void WriteVector(std::ofstream& f, const std::vector<T>& v) {
        Write<uint32_t>(f, (uint32_t)v.size());
        f.write(reinterpret_cast<const char*>(v.data()), (std::streamsize)(v.size() * sizeof(T)));
    }

    template<typename T>
    bool ReadVector(std::ifstream& f, std::vector<T>& v) {
        uint32_t size = 0;
        if (!Read(f, size) || size > MAX_ELEMENTS) return false;
        v.resize(size);
        f.read(reinterpret_cast<char*>(v.data()), (std::streamsize)((size_t)size * sizeof(T)));
        return static_cast<bool>(f);
    }

    void WriteHeader(std::ofstream& f, uint32_t magic) {
        Write(f, magic);
        Write(f, FORMAT_VERSION);
    }

    bool ReadHeader(std::ifstream& f, uint32_t magic) {
        uint32_t m = 0, version = 0;
        return Read(f, m) && Read(f, version) && m == magic && version == FORMAT_VERSION;
    }
}

// ---------------- Malla ----------------
bool WriteMeshFile(const std::string& path, const MeshData& d) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    WriteHeader(f, MESH_MAGIC);
    WriteString(f, d.name);
    WriteVector(f, d.vertices);
    WriteVector(f, d.indices);
    WriteVector(f, d.normals);
    return static_cast<bool>(f);
}

bool ReadMeshFile(const std::string& path, MeshData& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f || !ReadHeader(f, MESH_MAGIC)) return false;
    return ReadString(f, out.name) && ReadVector(f, out.vertices)
        && ReadVector(f, out.indices) && ReadVector(f, out.normals);
}

// ---------------- Textura ----------------
bool WriteTextureFile(const std::string& path, const TextureData& d) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    WriteHeader(f, TEXTURE_MAGIC);
    WriteString(f, d.name);
    Write<int32_t>(f, d.width);
    Write<int32_t>(f, d.height);
    WriteVector(f, d.pixels);
    return static_cast<bool>(f);
}

bool ReadTextureFile(const std::string& path, TextureData& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f || !ReadHeader(f, TEXTURE_MAGIC)) return false;

    int32_t w = 0, h = 0;
    if (!ReadString(f, out.name) || !Read(f, w) || !Read(f, h) || !ReadVector(f, out.pixels)) return false;
    if (w <= 0 || h <= 0 || out.pixels.size() != (size_t)w * (size_t)h * 4) return false;

    out.width = w;
    out.height = h;
    return true;
}

// ---------------- Modelo ----------------
bool WriteModelFile(const std::string& path, const ModelData& d) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    WriteHeader(f, MODEL_MAGIC);
    WriteString(f, d.name);
    Write<uint32_t>(f, (uint32_t)d.nodes.size());

    for (const ModelNode& n : d.nodes) {
        WriteString(f, n.name);
        Write<int32_t>(f, n.parent);
        Write<float>(f, n.position.x); Write<float>(f, n.position.y); Write<float>(f, n.position.z);
        Write<float>(f, n.rotation.x); Write<float>(f, n.rotation.y); Write<float>(f, n.rotation.z); Write<float>(f, n.rotation.w);
        Write<float>(f, n.scale.x); Write<float>(f, n.scale.y); Write<float>(f, n.scale.z);
        Write<uint64_t>(f, n.meshUUID);
        Write<uint64_t>(f, n.textureUUID);
    }
    return static_cast<bool>(f);
}

bool ReadModelFile(const std::string& path, ModelData& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f || !ReadHeader(f, MODEL_MAGIC)) return false;

    uint32_t count = 0;
    if (!ReadString(f, out.name) || !Read(f, count) || count > (1u << 20)) return false;

    out.nodes.resize(count);
    for (ModelNode& n : out.nodes) {
        int32_t parent = -1;
        float px, py, pz, qx, qy, qz, qw, sx, sy, sz;
        if (!ReadString(f, n.name) || !Read(f, parent)
            || !Read(f, px) || !Read(f, py) || !Read(f, pz)
            || !Read(f, qx) || !Read(f, qy) || !Read(f, qz) || !Read(f, qw)
            || !Read(f, sx) || !Read(f, sy) || !Read(f, sz)
            || !Read(f, n.meshUUID) || !Read(f, n.textureUUID)) return false;

        n.parent = parent;
        n.position = glm::vec3(px, py, pz);
        n.rotation = glm::quat(qw, qx, qy, qz);
        n.scale = glm::vec3(sx, sy, sz);
    }
    return true;
}

// ---------------- .meta ----------------
MetaFile::MetaFile(const std::string& path) : path(path) {
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const size_t eq = line.find('=');
        if (eq == std::string::npos || eq == 0) continue;
        values[line.substr(0, eq)] = line.substr(eq + 1);
    }
}

std::string MetaFile::GetString(const std::string& key, const std::string& fallback) const {
    const auto it = values.find(key);
    return it != values.end() ? it->second : fallback;
}

void MetaFile::SetString(const std::string& key, const std::string& value) {
    const auto it = values.find(key);
    if (it == values.end()) {
        values.emplace(key, value);
        dirty = true;
    }
    else if (it->second != value) {
        it->second = value;
        dirty = true;
    }
}

uint64_t MetaFile::GetUUID(const std::string& key) const {
    const std::string s = GetString(key);
    return s.empty() ? 0 : std::strtoull(s.c_str(), nullptr, 10);
}

void MetaFile::SetUUID(const std::string& key, uint64_t value) {
    SetString(key, std::to_string(value));
}

int MetaFile::GetInt(const std::string& key, int fallback) const {
    const std::string s = GetString(key);
    return s.empty() ? fallback : (int)std::strtol(s.c_str(), nullptr, 10);
}

void MetaFile::SetInt(const std::string& key, int value) {
    SetString(key, std::to_string(value));
}

bool MetaFile::Save() {
    if (!dirty) return true;

    std::ofstream f(path, std::ios::trunc);
    if (!f) return false;
    for (const auto& entry : values) f << entry.first << "=" << entry.second << "\n";

    dirty = false;
    return static_cast<bool>(f);
}