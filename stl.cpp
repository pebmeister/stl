//
// Written by Paul Baxter
//
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>
#include <memory>

#include "stl.h"

constexpr auto FACET_NAME_LEN = 5;
constexpr auto VERTEX_PER_TRIANGLE = 3;

// class constructor
// initialize member variables
stl::stl()
{
    // this will initialize all member variables
    cleanup();
}

// read_stl
// read a stl file 
// handles binary and ascii versions
int stl::read_stl(const char* name)
{
    auto result = 0;

    // by calling cleanup
    // multiple stls can be read 1 at a time in the same instance
    cleanup();
    m_name = std::string(name);
    if (!open_ascii())
    {
        cleanup();
        return -1;
    }
    
    // if 1st token is not 'solid' then its a binary stl
    get_next_token();
    if (strcmp(m_token, "solid") != 0) 
    { 
        if (!open_binary())
        {
            cleanup();
            return -1;
        }

        result = read_binary();
    }
    else 
    {
        // read to EOL
        read_line();
        // if the line starts with 'facet' then its an ascii stl
        if (strncmp(m_token, "facet", FACET_NAME_LEN) == 0)
        {
            m_stl_input_file.seekg(0, std::ios::beg);
            read_ascii();
        }
        else 
        {
            // the next token MUST be facet
            // for it to be an ascii stl
            get_next_token();
            if (strcmp(m_token, "facet") == 0) {
                m_stl_input_file.seekg(0, std::ios::beg);
                result = read_ascii();
            }
            else
            {
                if (!open_binary())
                {
                    cleanup();
                    return -1;
                }

                read_binary();
            }
        }
    }
    if (m_stl_input_file.is_open())
    {
        m_stl_input_file.close();
    }
    std::cout << 
        m_name.c_str() <<
        " triangles [" << m_num_triangles << "]" <<
        " vectors [" << m_vectors.size() << "]" <<
        " normals [" << m_normals.size() << "]" <<
        " rgb_colors [" << m_rgb_color.size() << "]\n";
    return 0;
}

// create a binary stl 
// caller should set up
// m_num_triangles, m_vectors, m_normals and m_rgb_color and m_header
// before calling
/*
* UINT8[80]    – Header - 80 bytes
* UINT32       – Number of triangles - 4 bytes
*
* foreach triangle - 50 bytes:
*    REAL32[3] – Normal vector - 12 bytes
*    REAL32[3] – Vertex 1 - 12 bytes
*    REAL32[3] – Vertex 2 - 12 bytes
*    REAL32[3] – Vertex 3 - 12 bytes
*    UINT16    – Attribute byte count - 2 bytes
* end
*/
int stl::create_stl_binary(const char* name)
{
    m_name = std::string(name);
    if (!open_write_binary())
    {
        cleanup();
        return -1;
    }

    if (m_num_triangles * 3LL != m_normals.size() || m_vectors.size() != m_normals.size() * 3LL)
    {
        std::cout << "Invalid stl data. " << 
            " triangles [" << m_num_triangles << "]" <<
            " vectors [" << m_vectors.size() << "]" <<
            " normals [" << m_normals.size() << "]" <<
            " rgb_colors [" << m_rgb_color.size() << "]\n";

        cleanup();
        return -1;
    }

    // write header
    m_stl_output_file.write(reinterpret_cast<char*>(m_header), STL_HEADER_SIZE);

    // write number of triangles
    m_stl_output_file.write(reinterpret_cast<char*>(&m_num_triangles), sizeof(m_num_triangles));

    unsigned long long normal_index = 0;
    unsigned long long vertex_index = 0;
    unsigned long long rgb_index = 0;

    for (uint32_t triangle = 0; triangle < m_num_triangles; triangle++)
    {
        float x = 0, y = 0, z = 0;

        x = m_normals[normal_index++];
        y = m_normals[normal_index++];
        z = m_normals[normal_index++];

        m_stl_output_file.write(reinterpret_cast<char*>(&x), sizeof(x));
        m_stl_output_file.write(reinterpret_cast<char*>(&y), sizeof(y));
        m_stl_output_file.write(reinterpret_cast<char*>(&z), sizeof(z));

        // read 3 vertices
        for (auto vert = 0; vert < VERTEX_PER_TRIANGLE; vert++)
        {
            x = m_vectors[vertex_index++];
            y = m_vectors[vertex_index++];
            z = m_vectors[vertex_index++];

            m_stl_output_file.write(reinterpret_cast<char*>(&x), sizeof(x));
            m_stl_output_file.write(reinterpret_cast<char*>(&y), sizeof(y));
            m_stl_output_file.write(reinterpret_cast<char*>(&z), sizeof(z));
        }
        uint16_t attribute = 0;
        if (rgb_index + 2 < m_rgb_color.size())
        {
            auto r = static_cast<uint16_t>(round(m_rgb_color[++rgb_index] * 128.0f));
            auto g = static_cast<uint16_t>(round(m_rgb_color[++rgb_index] * 128.0f));
            auto b = static_cast<uint16_t>(round(m_rgb_color[++rgb_index] * 128.0f));

            attribute |= (r & 0xF);
            attribute <<= 4;
            attribute |= (g & 0xF);
            attribute <<= 4;
            attribute |= (b & 0xF);
            attribute <<= 4;
            attribute |= 0x1;
        }
        m_stl_output_file.write(reinterpret_cast<char*>(&attribute), sizeof(attribute));
    }
    
    if (m_stl_output_file.is_open())
    {
        std::cout <<
            m_name.c_str() <<
            " triangles [" << m_num_triangles << "]" <<
            " vectors [" << m_vectors.size() << "]" <<
            " normals [" << m_normals.size() << "]" <<
            " rgb_colors [" << m_rgb_color.size() << "]\n";

        m_stl_output_file.close();
    }
    return 0;
}

// create an ascii stl 
// caller should set up
// m_num_triangles, m_vectors and m_normals
// before calling
/*
 * solid name
 * facet normal ni nj nk
 *    outer loop
 *       vertex v1x v1y v1z
 *       vertex v2x v2y v2z
 *       vertex v3x v3y v3z
 *   endloop
 * endfacet
 * endsolid name
 */
int stl::create_stl_ascii(const char* name)
{
    long long normal_index = 0;
    long long vertex_index = 0;

    m_name = std::string(name);
    if (!open_write_ascii())
    {
        cleanup();
        return -1;
    }

    if (m_num_triangles * 3LL != m_normals.size() || m_vectors.size() != m_normals.size() * 3LL)
    {
        m_stl_output_file << "Invalid stl data. " <<
            " triangles [" << m_num_triangles << "]" <<
            " vectors [" << m_vectors.size() << "]" <<
            " normals [" << m_normals.size() << "]" <<
            " rgb_colors [" << m_rgb_color.size() << "]\n";

        cleanup();
        return -1;
    }

    m_stl_output_file << "solid " << m_name << '\n';
    for (auto triangle = 0u; triangle < m_num_triangles; ++triangle)
    {
        float x, y, z;

        x = m_normals[normal_index++];
        y = m_normals[normal_index++];
        z = m_normals[normal_index++];
        m_stl_output_file << "facet normal " << x << ' ' << x << ' ' << z << '\n';
        
        m_stl_output_file << " outer loop\n";

        x = m_vectors[vertex_index++];
        y = m_vectors[vertex_index++];
        z = m_vectors[vertex_index++];
        m_stl_output_file << "  vertex " << x << ' ' << x << ' ' << z << '\n';

        x = m_vectors[vertex_index++];
        y = m_vectors[vertex_index++];
        z = m_vectors[vertex_index++];
        m_stl_output_file << "  vertex " << x << ' ' << x << ' ' << z << '\n';

        x = m_vectors[vertex_index++];
        y = m_vectors[vertex_index++];
        z = m_vectors[vertex_index++];
        m_stl_output_file << "  vertex " << x << ' ' << x << ' ' << z << '\n';

        m_stl_ou tput_file << " endloop\n";
        m_stl_output_file << "endfacet\n";
    }
    m_stl_output_file << "endsolid " << m_name.c_str() << '\n';

    return 0;
}

// read until end of line
char* stl::read_line()
{
    auto pos = 0ULL;
    m_token[pos] = 0;

    while (!m_stl_input_file.eof() && pos + 1 < sizeof(m_token)) 
    {
        char ch = m_stl_input_file.get();
        if (m_stl_input_file.eof() || ch == '\n')
        {
            break;
        }
        if (pos == 0 && ((ch < 0) || isspace(ch))) 
        {
            continue;
        }
        m_token[pos++] = ch;
    }
    m_token[pos] = 0;
    return m_token;
}

// get the token from the stl file
// return nullptr when at end of file
char* stl::get_next_token()
{
    auto pos = 0ULL;
    m_token[pos] = 0;

    while (!m_stl_input_file.eof() && pos + 1 < sizeof(m_token))
    {
        char ch = m_stl_input_file.get();

        if (m_stl_input_file.eof() || ch < 1 || std::isspace(ch)) {
            if (pos != 0) 
            {
                break;
            }
            else
            {
                continue;
            }
        }
        m_token[pos++] = ch;
    }
    m_token[pos] = 0;
    return m_token;
}

// validate_state
// make sure we are in expected parse state
// used in ascii stl
// return true if in expected state
bool stl::validate_state(const char* tok)
{
    if (strcmp(m_token, tok) != 0) 
    {
        std::cout << m_name.c_str() << " invalid stl file. expected [" << tok << "] but got [" << m_token << "].\n";
        m_cur_state = error;
        return false;
    }
    return true;
}

// Read an ascii stl file
/*
 * solid name
 * facet normal ni nj nk
 *    outer loop
 *       vertex v1x v1y v1z
 *       vertex v2x v2y v2z
 *       vertex v3x v3y v3z
 *   endloop
 * endfacet
 * endsolid name
 */
int stl::read_ascii() 
{
    auto result = 0;

    std::string solid_Name;
    char* tok = nullptr;
    do 
    {
        if (m_read_tok) 
        {
            tok = get_next_token();

            if (tok == nullptr)
                break;
        }

        switch (m_cur_state) 
        {
        case error:
            cleanup();
            return -1;

        case in_solid:
            m_read_tok = true;
            if (!validate_state("solid"))
            {
                result = -1;
                break;
            }
            m_cur_state = in_facet;
            tok = read_line();
            if (strncmp(tok, "facet", 5) == 0) 
            {
                m_read_tok = false;
                break;
            }
            solid_Name = tok;
            break;

        case in_facet:
            m_read_tok = true;
            if (tok != nullptr) 
            {
                if (strcmp(tok, "endsolid") == 0) 
                {
                    m_read_tok = false;
                    m_cur_state = in_endsolid;
                    break;
                }
            }
            if (!validate_state("facet")) 
            {
                result = -1;
                break;
            }
            m_cur_state = in_facet_normal;
            break;

        case in_facet_normal:
            m_read_tok = true;
            if (!validate_state("normal")) 
            {
                result = -1;
                break;
            }
            m_cur_state = in_facet_vertex_x;
            break;

        case in_facet_vertex_x:
        case in_facet_vertex_y:
        case in_facet_vertex_z:
            m_read_tok = true;
            read_facet_vertex();
            break;

        case in_outer:
            m_read_tok = true;
            if (!validate_state("outer")) 
            {
                result = -1;
                break;
            }
            m_cur_state = in_outer_loop;
            break;

        case in_outer_loop:
            m_read_tok = true;
            if (!validate_state("loop")) 
            {
                result = -1;
                break;
            }
            m_cur_state = in_vertex;
            break;

        case in_vertex:
            m_read_tok = true;
            if (!validate_state("vertex")) 
            {
                result = -1;
                break;
            }
            m_cur_state = in_vertex_x;
            break;

        case in_vertex_x:
        case in_vertex_y:
        case in_vertex_z:
            m_read_tok = true;
            read_vertex();
            break;

        case in_endloop:
            m_read_tok = true;
            if (!validate_state("endloop")) 
            {
                result = -1;
                break;
            }
            m_cur_state = in_endfacet;
            break;

        case in_endfacet:
            m_read_tok = true;
            if (!validate_state("endfacet")) 
            {
                result = -1;
                break;
            }
            m_cur_state = in_facet;
            break;

        case in_endsolid:
            m_read_tok = true;
            if (!validate_state("endsolid")) 
            {
                result = -1;
                break;
            }
            get_next_token();
            m_read_tok = false;
            tok = nullptr;
            break;
        }
    } while (tok != nullptr);

    m_num_triangles = static_cast<int16_t>(m_vectors.size() / 9);
    return result;
}

// read vertex in facet (the normal for vertex)
void stl::read_facet_vertex()
{
    char* ending = nullptr;
    auto vertex = strtof(m_token, &ending);
    m_normals.push_back(vertex);

    switch (m_cur_state) 
    {
    case in_facet_vertex_x:
        m_cur_state = in_facet_vertex_y;
        break;

    case in_facet_vertex_y:
        m_cur_state = in_facet_vertex_z;
        break;

    case in_facet_vertex_z:
        m_cur_state = in_outer;
        break;

    default:
        std::cout << m_name.c_str() << " invalid stl file. Expected a vertex normal value.\n";
        m_cur_state = error;
        m_read_tok = false;
        break;
    }
}

// read vertex in stl ascii file
void stl::read_vertex()
{
    char* ending = nullptr;
    auto v = strtof(m_token, &ending);
    m_vectors.push_back(v);
    m_read_tok = true;

    switch (m_cur_state)
    {
    case in_vertex_x:
        m_cur_state = in_vertex_y;
        break;

    case in_vertex_y:
        m_cur_state = in_vertex_z;
        break;

    case in_vertex_z:
        m_read_tok = true;
        get_next_token();
        m_read_tok = false;
        m_cur_state = strcmp(m_token, "endloop") == 0 ? in_endloop : in_vertex;
        break;

    default:
        std::cout << m_name.c_str() << " invalid stl file. Expected a vertex value.\n";
        m_cur_state = error;
        m_read_tok = false;
        break;
    }
}

/*
 * read_binary
 *
 * UINT8[80]    – Header                 -     80 bytes
 * UINT32       – Number of triangles    -      4 bytes
 *
 * foreach triangle                      -      50 bytes:
 *      REAL32[3] – Normal vector        -      12 bytes
 *      REAL32[3] – Vertex 1             -      12 bytes
 *      REAL32[3] – Vertex 2             -      12 bytes
 *      REAL32[3] – Vertex 3             -      12 bytes
 *      UINT16    – Attribute byte count -      2 bytes
 * end
 */
int stl::read_binary() 
{
    auto result = 0;
    if (m_size < STL_HEADER_SIZE + sizeof(m_num_triangles)) 
    {
        std::cout << m_name.c_str() << " invalid stl file.\n";
        cleanup();
        return -1;
    }

    // read header
    m_stl_input_file.read(reinterpret_cast<char*>(m_header), STL_HEADER_SIZE);

    // read number of triangles
    m_stl_input_file.read(reinterpret_cast<char*>(&m_num_triangles), sizeof(m_num_triangles));

    // Check size
    if (static_cast<long long>(STL_HEADER_SIZE + sizeof(int32_t) + static_cast<long long>(m_num_triangles * STL_TRIANGLE_SIZE)) < m_size)
    {
        std::cout << m_name.c_str() << " invalid stl file.\n";
        cleanup();        
        return -1;
    }

    for (uint32_t triangle = 0; triangle < m_num_triangles; triangle++) 
    {
        float x = 0, y = 0, z = 0;

        // read normal vector
        m_stl_input_file.read(reinterpret_cast<char*>(&x), sizeof(x));
        m_stl_input_file.read(reinterpret_cast<char*>(&y), sizeof(y));
        m_stl_input_file.read(reinterpret_cast<char*>(&z), sizeof(z));

        m_normals.push_back(x);
        m_normals.push_back(y);
        m_normals.push_back(z);

        // read 3 vertices
        for (auto vert = 0; vert < VERTEX_PER_TRIANGLE; vert++)
        {
            // read vertex

            m_stl_input_file.read(reinterpret_cast<char*>(&x), sizeof(x));
            m_stl_input_file.read(reinterpret_cast<char*>(&y), sizeof(y));
            m_stl_input_file.read(reinterpret_cast<char*>(&z), sizeof(z));

            m_vectors.push_back(x);
            m_vectors.push_back(y);
            m_vectors.push_back(z);
        }

        // read attribute
        // 4 bits for each part
        uint16_t attribute = 0;
        m_stl_input_file.read(reinterpret_cast<char*>(&attribute), sizeof(attribute));

        uint16_t b = attribute & 0xF;
        attribute >>= 4;
        uint16_t g = attribute & 0xF;
        attribute >>= 4;
        uint16_t r = attribute & 0xF;
        attribute >>= 4;
        auto valid = attribute & 0x1;

        if (valid)
        {
            //  convert rgb values to 0 - 1
            constexpr float mask = 0x0F;
            m_rgb_color.push_back(static_cast<float>(r) / mask);
            m_rgb_color.push_back(static_cast<float>(g) / mask);
            m_rgb_color.push_back(static_cast<float>(b) / mask);
        }
    }

    auto pos = m_stl_input_file.tellg();
    m_stl_input_file.close();

    if (pos != m_size) 
    {
        std::cout << m_name.c_str() << " unexpected data at and of file.";
    }
    return result;
}

// open or repopen stl file in binary mode
bool stl::open_binary()
{
    return open_read_common(std::ios::binary);
}

// open or repopen stl file in ascii mode
bool stl::open_ascii()
{
    return open_read_common(0);
}

// open or repopen stl file in selected mode
bool stl::open_read_common(int mode)
{
    // binary file. Close it if open and reopen in selected mode
    if (m_stl_input_file.is_open())
    {
        m_stl_input_file.close();
    }
    m_stl_input_file.open(m_name.c_str(), mode | std::ios::ate);
    if (!m_stl_input_file.is_open())
    {
        std::cout << "Unable to open stl file " << m_name.c_str() << ".\n";
        return false;
    }
    // get the size of the file
    m_size = m_stl_input_file.tellg();
    if (m_size < MIN_STL_LENGTH)
    {
        std::cout << m_name.c_str() << " invalid stl file.\n";
        return false;
    }
    m_stl_input_file.seekg(0, std::ios::beg);
    return true;
}

// open or repopen stl file in binary mode
bool stl::open_write_binary()
{
    return open_write_common(std::ios::binary | std::ios::trunc);
}

// open or repopen stl file in ascii mode
bool stl::open_write_ascii()
{
    return open_write_common(std::ios::trunc);
}

// open or repopen stl file in selected mode
bool stl::open_write_common(int mode)
{
    // binary file. Close it if open and reopen in binary mode
    if (m_stl_output_file.is_open())
    {
        m_stl_output_file.close();
    }
    m_stl_output_file.open(m_name.c_str(), mode);
    if (!m_stl_output_file.is_open())
    {
        std::cout << "Unable to open stl file " << m_name.c_str() << ".\n";
        return false;
    }

    return true;
}

// cleanup
// close files if open
// intialize member variables
void stl::cleanup()
{
    if (m_stl_input_file.is_open())
    {
        m_stl_input_file.close();
    }
    if (m_stl_output_file.is_open())
    {
        m_stl_output_file.close();
    }
    m_vectors.clear();
    m_normals.clear();
    m_rgb_color.clear();
    memset(m_header, 0, STL_HEADER_SIZE);

    m_num_triangles = 0;
    m_size = 0;
    m_read_tok = true;
    m_cur_state = in_solid;
}

stl::~stl()
{
    cleanup();
}
