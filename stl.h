#pragma once
//
// Created by Paul Baxter on 11/17/2024.
//

#ifndef STL_H
#define STL_H

#include <fstream>
#include <vector>

constexpr int STL_HEADER_SIZE = 80U;
constexpr int STL_TRIANGLE_SIZE = 50;
constexpr int MAX_TOKEN_LEN = 1024;
constexpr int MIN_STL_LENGTH = 6;
constexpr int FACET_NAME_LEN = 5;
constexpr int VERTEX_PER_TRIANGLE = 3;
constexpr int AXIS_PER_VERTEX = 3;

class stl
{
public:
    uint32_t m_num_triangles;
    std::vector<float> m_vectors;
    std::vector<float> m_normals;
    std::vector<float> m_rgb_color;
    std::streamoff m_size;
    char m_header[STL_HEADER_SIZE] = { 0 };

    stl();
    ~stl();

    int read_stl(const char* name);
    int create_stl_binary(const char* name);
    int create_stl_ascii(const char* name);
    void calc_normals();

private:

    enum sti_parse_state
    {
        error,
        in_solid,
        in_facet,
        in_facet_normal,
        in_facet_vertex_x,
        in_facet_vertex_y,
        in_facet_vertex_z,
        in_outer,
        in_outer_loop,
        in_vertex,
        in_vertex_x,
        in_vertex_y,
        in_vertex_z,
        in_endloop,
        in_endfacet,
        in_endsolid
    };

    std::string m_name;
    std::ifstream m_stl_input_file;
    std::ofstream m_stl_output_file;
    bool m_read_tok = true;
    sti_parse_state m_cur_state = in_solid;
    char m_token[MAX_TOKEN_LEN] = { 0 };

    void cleanup();
    int read_binary();
    int read_ascii();
    char* get_next_token();
    char* read_line();
    void read_facet_vertex();
    void read_vertex();
    bool validate_state(const char* msg);
    bool open_read_common(int mode);
    bool open_binary();
    bool open_ascii();
    bool open_write_common(int mode);
    bool open_write_binary();
    bool open_write_ascii();
};

#endif
