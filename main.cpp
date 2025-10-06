// main.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include <iomanip>
#include "stl.h"

// return the root name if a file
static const char* short_name(const char* name)
{
    auto len = strlen(name);
    auto p = &name[len];
    while (p > name) {
        if (*p == '\\' || *p == '/')
            return p + 1;
        p--;
    }
    return name;
}

std::vector<float> Pyramid_vectors =
{
    // front triangle
    0, -1, 0,       // top
    -1, 0, 1,     	// left front
    1, 0, 1,     	// right front

    // right triangle
    0, -1, 0,       // top
    1, 0, 1,     	// right front
    1, 0, -1,       // right back

    // back triangle
    0, -1, 0,       // top
    1, 0, -1,  	    // right back
    -1, 0, -1,   	// left back

    // left triangle
    0, -1, 0,       // top
    -1, 0, -1,      // left back
    -1, 0, 1        // left front 
};

int main(int argc, char* argv[])
{
    stl stlfile;
    for (auto arg = 1; arg < argc; ++arg) {
        try {
            stlfile.read_stl(argv[arg]);
        } catch (const std::exception& ex) {
            std::cerr << "Error in read_stl: " << ex.what() << std::endl;
            return 1;
        } catch (...) {
            std::cerr << "Unknown error in read_stl." << std::endl;
            return 1;
        }

        std::cout << "[" << std::setw(20) << short_name(argv[arg]) << "]  " <<
            " Triangles " << stlfile.m_num_triangles <<
            " Vectors " << stlfile.m_vectors.size() <<
            " Normals " << stlfile.m_normals.size() <<
            " RGBColors " << stlfile.m_rgb_color.size() << "\n";
    }

    stlfile.m_num_triangles = 0;
    stlfile.m_vectors.clear();
    stlfile.m_normals.clear();
    stlfile.m_rgb_color.clear();
    memset(stlfile.m_header, 0, STL_HEADER_SIZE);

    try {
        strcpy(stlfile.m_header, "Pauls Pyramid");
        for (auto& v : Pyramid_vectors) {
            stlfile.m_vectors.push_back(v);
        }
        stlfile.calc_normals();
    } catch (const std::exception& ex) {
        std::cerr << "Error in pyramid setup or calc_normals: " << ex.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error in pyramid setup or calc_normals." << std::endl;
        return 1;
    }

    try {
        stlfile.create_stl_binary("pyramid_bin.stl");
    } catch (const std::exception& ex) {
        std::cerr << "Error in create_stl_binary: " << ex.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error in create_stl_binary." << std::endl;
        return 1;
    }

    try {
        stlfile.read_stl("pyramid_bin.stl");
    } catch (const std::exception& ex) {
        std::cerr << "Error in read_stl (pyramid_bin.stl): " << ex.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error in read_stl (pyramid_bin.stl)." << std::endl;
        return 1;
    }

    std::cout << "[" << std::setw(20) << "pyramid_bin.stl" << "]  " <<
        " Triangles " << stlfile.m_num_triangles <<
        " Vectors " << stlfile.m_vectors.size() <<
        " Normals " << stlfile.m_normals.size() <<
        " RGBColors " << stlfile.m_rgb_color.size() << "\n";

    try {
        stlfile.create_stl_ascii("pyramid_ascii.stl");
    } catch (const std::exception& ex) {
        std::cerr << "Error in create_stl_ascii: " << ex.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error in create_stl_ascii." << std::endl;
        return 1;
    }

    try {
        stlfile.read_stl("pyramid_ascii.stl");
    } catch (const std::exception& ex) {
        std::cerr << "Error in read_stl (pyramid_ascii.stl): " << ex.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error in read_stl (pyramid_ascii.stl)." << std::endl;
        return 1;
    }

    std::cout << "[" << std::setw(20) << "pyramid_ascii.stl" << "]  " <<
        " Triangles " << stlfile.m_num_triangles <<
        " Vectors " << stlfile.m_vectors.size() <<
        " Normals " << stlfile.m_normals.size() <<
        " RGBColors " << stlfile.m_rgb_color.size() << "\n";
}
