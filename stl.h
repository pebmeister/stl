#pragma once
//
// Created by Paul Baxter on 11/17/2024.
//
#pragma once
#include <cstdint>
#include <vector>

class stl {
public:
    // Flattened triangle vertex array: [x0,y0,z0, x1,y1,z1, x2,y2,z2, ...]
    std::vector<float> m_vectors;

    // Optional per-vertex RGB colors (not required by STL format, but kept for compatibility)
    std::vector<float> m_rgb_color;

    // Number of triangles
    std::uint32_t m_num_triangles = 0;

public:
    // Reads ASCII or Binary STL. Returns 0 on success.
    // On any error, throws std::runtime_error (no console output).
    int read_stl(const char* path);

    // Centers geometry at origin and normalizes so largest extent == 1.0
    void normalizeAndCenter();

    // Clear all data
    void clear();
};

