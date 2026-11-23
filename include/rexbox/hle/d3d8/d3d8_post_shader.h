// Adapted from re2DJ (BSD 3-Clause License; see THIRD_PARTY_NOTICES.md).
// Copyright (c) 2026, re2DJ contributors

#ifndef REXBOX_HLE_D3D8_D3D8_POST_SHADER_H
#define REXBOX_HLE_D3D8_D3D8_POST_SHADER_H

#include "rexbox/hle/d3d8/post_shader_catalog.h"
#include "rexbox/hle/d3d8/post_shader_source.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace rexbox::hle::d3d8
{

// Runtime control interface for the presentation post-processing shader pass.
// Every method is called only from the overlay builder while the OpenGL context
// is current on the presenting thread.
class PostShaderControl
{
public:
    virtual ~PostShaderControl() = default;

    virtual const std::vector<PostShaderEntry>& catalog() const = 0;
    virtual const std::string& shader_directory() const = 0;

    virtual const std::string& active_id() const = 0;
    virtual const std::string& last_error() const = 0;

    virtual bool Select(std::string_view id) = 0;
    virtual bool Reload() = 0;

    virtual std::vector<PostShaderParameter>& parameters() = 0;
};

// Configured by the application before the D3D8 device is created.
void SetPostShaderDirectory(const std::filesystem::path& directory);
std::filesystem::path GetPostShaderDirectory();
void SetInitialPostShader(std::string id);

// Returns the active control object when a GL device exists, or nullptr
// otherwise. Call its methods only from the overlay builder.
PostShaderControl* GetPostShaderControl();

}  // namespace rexbox::hle::d3d8

#endif  // REXBOX_HLE_D3D8_D3D8_POST_SHADER_H
