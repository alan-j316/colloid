#pragma once
/**
 * @file GLTFParser.h
 * @author Alan Abraham P Kochumon
 * @date Created on: November 18, 2026
 *
 * @brief GLTF Parsing.
 *
 * @copyright Copyright (c) 2026 Alan Abraham P Kochumon
 */

#include <falcon/Falcon.h>
#include <vector>


namespace colloid
{
    struct Model
    {
        std::vector<flcn::Vec3F> positions;
        std::vector<uint32_t> indices;
        std::vector<flcn::CVec2F> uvs;
        std::vector<flcn::Vec3F> normal;
    };

    Model parseGLTF(std::string_view path) noexcept;

}