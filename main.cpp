// main.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "stl.h"

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
        stlfile.read_stl(argv[arg]);
        std::cout << "[" << arg << "]  " <<
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

    strcpy(stlfile.m_header, "Pauls Pyramid");
    for (auto& v : Pyramid_vectors) {
        stlfile.m_vectors.push_back(v);
    }
    stlfile.calc_normals();

    stlfile.create_stl_binary("pyramid_bin.stl");
    stlfile.read_stl("pyramid_bin.stl");
    std::cout << "[" << argc + 0 << "]  " <<
        " Triangles " << stlfile.m_num_triangles <<
        " Vectors " << stlfile.m_vectors.size() <<
        " Normals " << stlfile.m_normals.size() <<
        " RGBColors " << stlfile.m_rgb_color.size() << "\n";

    stlfile.create_stl_ascii("pyramid_ascii.stl");
    stlfile.read_stl("pyramid_ascii.stl");
    std::cout << "[" << argc + 1 << "]  " <<
        " Triangles " << stlfile.m_num_triangles <<
        " Vectors " << stlfile.m_vectors.size() <<
        " Normals " << stlfile.m_normals.size() <<
        " RGBColors " << stlfile.m_rgb_color.size() << "\n";
}
