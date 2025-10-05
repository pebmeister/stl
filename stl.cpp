// Written by Paul Baxter
//
#include "stl.h"

#include <stdexcept>
#include <string>
#include <fstream>
#include <sstream>
#include <cstring>
#include <limits>
#include <algorithm>

namespace {

// Binary STL layout helpers
constexpr std::size_t kBinaryHeaderSize = 80;
constexpr std::size_t kBinaryCountSize  = 4;
constexpr std::size_t kBinaryTriSize    = 50; // 12 (normal) + 36 (3 vertices) + 2 (attr count)

inline float read_f32_le(const unsigned char* p)
{
    float v;
    std::memcpy(&v, p, sizeof(float));
    return v;
}

inline std::uint32_t read_u32_le(const unsigned char* p)
{
    std::uint32_t v;
    std::memcpy(&v, p, sizeof(std::uint32_t));
    return v;
}

inline bool looks_like_binary_stl(const std::vector<unsigned char>& data, std::string& why_not)
{
    if (data.size() < kBinaryHeaderSize + kBinaryCountSize) {
        why_not = "File smaller than minimal binary STL header.";
        return false;
    }
    const auto tri_count = read_u32_le(data.data() + kBinaryHeaderSize);
    const std::size_t expected_size = kBinaryHeaderSize + kBinaryCountSize + static_cast<std::size_t>(tri_count) * kBinaryTriSize;
    if (data.size() == expected_size) {
        return true;
    }
    why_not = "Size does not match binary STL formula.";
    return false;
}

inline void parse_binary_stl(const std::vector<unsigned char>& data,
                             std::vector<float>& out_vertices,
                             std::uint32_t& out_triangles)
{
    const std::uint32_t tri_count = read_u32_le(data.data() + kBinaryHeaderSize);
    const unsigned char* ptr = data.data() + kBinaryHeaderSize + kBinaryCountSize;

    out_vertices.clear();
    out_vertices.reserve(static_cast<std::size_t>(tri_count) * 9);
    out_triangles = 0;

    for (std::uint32_t i = 0; i < tri_count; ++i) {
        // skip normal (12 bytes)
        ptr += 12;

        // 3 vertices
        for (int v = 0; v < 3; ++v) {
            float x = read_f32_le(ptr +  0);
            float y = read_f32_le(ptr +  4);
            float z = read_f32_le(ptr +  8);
            out_vertices.push_back(x);
            out_vertices.push_back(y);
            out_vertices.push_back(z);
            ptr += 12;
        }

        // attribute byte count (2 bytes)
        ptr += 2;
        ++out_triangles;
    }
}

inline void parse_ascii_stl(const std::string& text,
                            std::vector<float>& out_vertices,
                            std::uint32_t& out_triangles)
{
    std::istringstream iss(text);
    std::string token;
    out_vertices.clear();
    out_triangles = 0;

    // We extract every "vertex x y z" triple we find.
    // Each 3 vertices encountered form one triangle in order of appearance.
    int vertex_mod = 0;
    float x = 0, y = 0, z = 0;

    while (iss >> token) {
        if (token == "vertex") {
            if (!(iss >> x >> y >> z)) {
                throw std::runtime_error("ASCII STL parse error: malformed vertex line.");
            }
            out_vertices.push_back(x);
            out_vertices.push_back(y);
            out_vertices.push_back(z);
            vertex_mod++;
            if (vertex_mod == 3) {
                // We just completed a triangle
                out_triangles++;
                vertex_mod = 0;
            }
        }
        // else ignore other tokens
    }

    if (!out_vertices.empty() && (out_vertices.size() % 9) != 0) {
        throw std::runtime_error("ASCII STL parse error: vertex count is not a multiple of 3.");
    }
}

} // namespace

int stl::read_stl(const char* path)
{
    if (!path || !*path) {
        throw std::runtime_error("STL read error: empty path.");
    }

    // Read entire file into memory
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        throw std::runtime_error(std::string("STL read error: cannot open '") + path + "'.");
    }

    ifs.seekg(0, std::ios::end);
    const std::streampos file_size = ifs.tellg();
    if (file_size <= 0) {
        throw std::runtime_error(std::string("STL read error: '") + path + "' has zero size.");
    }
    ifs.seekg(0, std::ios::beg);

    std::vector<unsigned char> data(static_cast<std::size_t>(file_size));
    if (!ifs.read(reinterpret_cast<char*>(data.data()), file_size)) {
        throw std::runtime_error(std::string("STL read error: failed to read '") + path + "'.");
    }

    // Decide ASCII vs Binary:
    // Prefer binary if size matches the canonical formula; otherwise parse ASCII.
    std::string why_not_binary;
    bool is_binary = looks_like_binary_stl(data, why_not_binary);

    try {
        if (is_binary) {
            parse_binary_stl(data, m_vectors, m_num_triangles);
        } else {
            // Treat as ASCII
            std::string text(reinterpret_cast<const char*>(data.data()), data.size());
            parse_ascii_stl(text, m_vectors, m_num_triangles);
        }
    } catch (const std::exception& e) {
        // Attach filename context and rethrow
        std::string msg = std::string("STL parse error in '") + path + "': " + e.what();
        throw std::runtime_error(msg);
    }

    // Clear any stale colors; geometry may be reloaded
    m_rgb_color.clear();

    // Success
    return 0;
}

void stl::normalizeAndCenter()
{
    if (m_vectors.empty()) return;

    float minx = std::numeric_limits<float>::infinity();
    float miny = std::numeric_limits<float>::infinity();
    float minz = std::numeric_limits<float>::infinity();
    float maxx = -std::numeric_limits<float>::infinity();
    float maxy = -std::numeric_limits<float>::infinity();
    float maxz = -std::numeric_limits<float>::infinity();

    const std::size_t vcount = m_vectors.size() / 3;
    for (std::size_t i = 0; i < vcount; ++i) {
        float x = m_vectors[i * 3 + 0];
        float y = m_vectors[i * 3 + 1];
        float z = m_vectors[i * 3 + 2];
        minx = std::min(minx, x); maxx = std::max(maxx, x);
        miny = std::min(miny, y); maxy = std::max(maxy, y);
        minz = std::min(minz, z); maxz = std::max(maxz, z);
    }

    const float cx = 0.5f * (minx + maxx);
    const float cy = 0.5f * (miny + maxy);
    const float cz = 0.5f * (minz + maxz);

    const float sx = (maxx - minx);
    const float sy = (maxy - miny);
    const float sz = (maxz - minz);
    const float span = std::max({ sx, sy, sz, 1e-12f });

    for (std::size_t i = 0; i < vcount; ++i) {
        float& x = m_vectors[i * 3 + 0];
        float& y = m_vectors[i * 3 + 1];
        float& z = m_vectors[i * 3 + 2];
        x = (x - cx) / span;
        y = (y - cy) / span;
        z = (z - cz) / span;
    }
}

void stl::clear()
{
    m_vectors.clear();
    m_rgb_color.clear();
    m_num_triangles = 0;
}

