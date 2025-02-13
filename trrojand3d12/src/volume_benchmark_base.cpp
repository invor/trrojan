﻿// <copyright file="volume_benchmark_base.cpp" company="Visualisierungsinstitut der Universität Stuttgart">
// Copyright © 2016 - 2023 Visualisierungsinstitut der Universität Stuttgart.
// Licensed under the MIT licence. See LICENCE.txt file in the project root for full licence information.
// </copyright>
// <author>Christoph Müller</author>

#include "trrojan/d3d12/volume_benchmark_base.h"

#include <cassert>
#include <stdexcept>

#include "trrojan/brudervn_xfer_func.h"
#include "trrojan/clipping.h"
#include "trrojan/io.h"
#include "trrojan/log.h"
#include "trrojan/text.h"

#include "trrojan/d3d12/utilities.h"


#define _VOLUME_BENCH_DEFINE_FACTOR(f)                                         \
const char *trrojan::d3d12::volume_benchmark_base::factor_##f = #f

_VOLUME_BENCH_DEFINE_FACTOR(data_set);
_VOLUME_BENCH_DEFINE_FACTOR(ert_threshold);
_VOLUME_BENCH_DEFINE_FACTOR(frame);
_VOLUME_BENCH_DEFINE_FACTOR(fovy_deg);
_VOLUME_BENCH_DEFINE_FACTOR(gpu_counter_iterations);
_VOLUME_BENCH_DEFINE_FACTOR(max_steps);
_VOLUME_BENCH_DEFINE_FACTOR(min_prewarms);
_VOLUME_BENCH_DEFINE_FACTOR(min_wall_time);
_VOLUME_BENCH_DEFINE_FACTOR(step_size);
_VOLUME_BENCH_DEFINE_FACTOR(xfer_func);

#undef _VOLUME_BENCH_DEFINE_FACTOR


/*
 * trrojan::d3d12::volume_benchmark_base::get_format
 */
DXGI_FORMAT trrojan::d3d12::volume_benchmark_base::get_format(
        const info_type& info) {
    // Note: this is identical with the D3D11 case.
    auto resolution = info.resolution();
    if (resolution.size() != 3) {
        throw std::invalid_argument("The given data set is not a 3D volume.");
    }

    auto components = info.components();
    if ((components < 1) || (components > 4)) {
        throw std::invalid_argument("The number of per-voxel components of the "
            "given data set is not within [1, 4]");
    }

    switch (info.format()) {
        case datraw::scalar_type::int8:
            switch (components) {
                case 1: return DXGI_FORMAT_R8_SNORM;
                case 2: return DXGI_FORMAT_R8G8_SNORM;
                case 4: return DXGI_FORMAT_R8G8B8A8_SNORM;
            }
            break;

        case datraw::scalar_type::int16:
            switch (components) {
                case 1: return DXGI_FORMAT_R16_SNORM;
                case 2: return DXGI_FORMAT_R16G16_SNORM;
                case 4: return DXGI_FORMAT_R16G16B16A16_SNORM;
            }
            break;

        case datraw::scalar_type::int32:
            switch (components) {
                case 1: return DXGI_FORMAT_R32_SINT;
                case 2: return DXGI_FORMAT_R32G32_SINT;
                case 3: return DXGI_FORMAT_R32G32B32_SINT;
                case 4: return DXGI_FORMAT_R32G32B32A32_SINT;
            }
            break;

        case datraw::scalar_type::uint8:
            switch (components) {
                case 1: return DXGI_FORMAT_R8_UNORM;
                case 2: return DXGI_FORMAT_R8G8_UNORM;
                case 4: return DXGI_FORMAT_R8G8B8A8_UNORM;
            }
            break;

        case datraw::scalar_type::uint16:
            switch (components) {
                case 1: return DXGI_FORMAT_R16_UNORM;
                case 2: return DXGI_FORMAT_R16G16_UNORM;
                case 4: return DXGI_FORMAT_R16G16B16A16_UNORM;
            }
            break;

        case datraw::scalar_type::uint32:
            switch (components) {
                case 1: return DXGI_FORMAT_R32_UINT;
                case 2: return DXGI_FORMAT_R32G32_UINT;
                case 3: return DXGI_FORMAT_R32G32B32_UINT;
                case 4: return DXGI_FORMAT_R32G32B32A32_UINT;
            }
            break;

        case datraw::scalar_type::float16:
            switch (components) {
                case 1: return DXGI_FORMAT_R16_FLOAT;
                case 2: return DXGI_FORMAT_R16G16_FLOAT;
                case 4: return DXGI_FORMAT_R16G16B16A16_FLOAT;
            }
            break;

        case datraw::scalar_type::float32:
            switch (components) {
                case 1: return DXGI_FORMAT_R32_FLOAT;
                case 2: return DXGI_FORMAT_R32G32_FLOAT;
                case 3: return DXGI_FORMAT_R32G32B32_FLOAT;
                case 4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
            }
            break;
    }

    throw std::invalid_argument("The given scalar data type is unknown or "
        "unsupported.");
}


/*
 * trrojan::d3d12::volume_benchmark_base::load_brudervn_xfer_func
 */
winrt::com_ptr<ID3D12Resource>
trrojan::d3d12::volume_benchmark_base::load_brudervn_xfer_func(
        const std::string& path,
        d3d12::device& device,
        ID3D12GraphicsCommandList *cmd_list,
        const D3D12_RESOURCE_STATES state,
        winrt::com_ptr<ID3D12Resource>& out_staging) {
    log::instance().write_line(log_level::debug, "Loading transfer function "
        "from {} ...", path);
    const auto data = trrojan::load_brudervn_xfer_func(path);
    return load_xfer_func(data, device, cmd_list, state, out_staging);
}


/*
 * trrojan::d3d12::volume_benchmark_base::load_volume
 */
winrt::com_ptr<ID3D12Resource>
trrojan::d3d12::volume_benchmark_base::load_volume(
        const std::string& path,
        const frame_type frame,
        d3d12::device& device,
        ID3D12GraphicsCommandList *cmd_list,
        const D3D12_RESOURCE_STATES state,
        info_type& out_info,
        winrt::com_ptr<ID3D12Resource>& out_staging) {
    log::instance().write_line(log_level::debug, "Loading volume data "
        "from {} ...", path);
    auto reader = reader_type::open(path);

    if (!reader.move_to(frame)) {
        throw std::invalid_argument("The given frame number does not exist.");
    }

    auto resolution = reader.info().resolution();
    if (resolution.size() != 3) {
        throw std::invalid_argument("The given data set is not a 3D volume.");
    }

    if (cmd_list == nullptr) {
        throw std::invalid_argument("A valid command list for uploading the "
            "volume data set must be provided.");
    }

    // Stage the volume.
    const auto data = reader.read_current();
    out_staging = create_upload_buffer(device.d3d_device().get(), data.size());
    set_debug_object_name(out_staging, "volume_staging");
    stage_data(out_staging.get(), data.data(), data.size());

    // Copy the data from the staging buffer to the texture.
    const auto format = get_format(reader.info());
    auto retval = create_texture(device.d3d_device(), resolution[0],
        resolution[1], resolution[2], format);
    set_debug_object_name(retval, "volume");

    auto src_loc = get_copy_location(out_staging.get());
    assert(src_loc.PlacedFootprint.Footprint.Format == DXGI_FORMAT_UNKNOWN);
    src_loc.PlacedFootprint.Footprint.Format = format;
    src_loc.PlacedFootprint.Footprint.Width = resolution[0];
    src_loc.PlacedFootprint.Footprint.Height = resolution[1];
    src_loc.PlacedFootprint.Footprint.Depth = resolution[2];
    src_loc.PlacedFootprint.Footprint.RowPitch = reader.info().row_pitch();
    auto dst_loc = get_copy_location(retval.get());

    cmd_list->CopyTextureRegion(&dst_loc, 0, 0, 0, &src_loc, nullptr);
    transition_resource(cmd_list, retval.get(), D3D12_RESOURCE_STATE_COPY_DEST,
        state);

    // Return the info block in case of success.
    out_info = reader.info();

    return retval;
}


/*
 * trrojan::d3d12::volume_benchmark_base::load_xfer_func
 */
winrt::com_ptr<ID3D12Resource>
trrojan::d3d12::volume_benchmark_base::load_xfer_func(
        const std::vector<std::uint8_t>& data,
        d3d12::device& device,
        ID3D12GraphicsCommandList *cmd_list,
        const D3D12_RESOURCE_STATES state,
        winrt::com_ptr<ID3D12Resource>& out_staging) {
    const auto cnt = std::div(static_cast<long>(data.size()), 4l);
    const auto format = DXGI_FORMAT_R8G8B8A8_UNORM;

    if (cnt.rem != 0) {
        throw std::invalid_argument("The transfer function texture does not "
            "hold valid data in DXGI_FORMAT_R8G8B8A8_UNORM.");
    }

    if (cmd_list == nullptr) {
        throw std::invalid_argument("A valid command list for uploading the "
            "transfer function must be provided.");
    }

    // Create a staging buffer for uploading the data.
    out_staging = create_upload_buffer(device.d3d_device().get(), data.size());
    set_debug_object_name(out_staging, "xfer_func_staging");
    stage_data(out_staging.get(), data.data(), data.size());

    // Copy the data from the staging buffer to the texture.
    auto retval = create_texture(device.d3d_device(), cnt.quot, format);
    set_debug_object_name(retval, "xfer_func");

    auto src_loc = get_copy_location(out_staging.get());
    assert(src_loc.PlacedFootprint.Footprint.Format == DXGI_FORMAT_UNKNOWN);
    src_loc.PlacedFootprint.Footprint.Format = format;
    assert(src_loc.PlacedFootprint.Footprint.Width > data.size());
    src_loc.PlacedFootprint.Footprint.Width
        = static_cast<UINT>(data.size() / 4);
    auto dst_loc = get_copy_location(retval.get());

    cmd_list->CopyTextureRegion(&dst_loc, 0, 0, 0, &src_loc, nullptr);
    transition_resource(cmd_list, retval.get(), D3D12_RESOURCE_STATE_COPY_DEST,
        state);

    return retval;
}


/*
 * trrojan::d3d12::volume_benchmark_base::load_xfer_func
 */
winrt::com_ptr<ID3D12Resource>
trrojan::d3d12::volume_benchmark_base::load_xfer_func(
        const std::string &path,
        d3d12::device& device,
        ID3D12GraphicsCommandList *cmd_list,
        const D3D12_RESOURCE_STATES state,
        winrt::com_ptr<ID3D12Resource>& out_staging) {
    log::instance().write_line(log_level::debug, "Loading transfer function "
        "from {} ...", path);
    const auto data = read_binary_file(path);
    return load_xfer_func(data, device, cmd_list, state, out_staging);
}


/*
 * trrojan::d3d12::volume_benchmark_base::load_xfer_func
 */
winrt::com_ptr<ID3D12Resource>
trrojan::d3d12::volume_benchmark_base::load_xfer_func(
        const configuration& config,
        d3d12::device& device,
        ID3D12GraphicsCommandList *cmd_list,
        const D3D12_RESOURCE_STATES state,
        winrt::com_ptr<ID3D12Resource>& out_staging) {
    try {
        auto path = config.get<std::string>(factor_xfer_func);

        if (ends_with(path, std::string(".brudervn"))) {
            return volume_benchmark_base::load_brudervn_xfer_func(
                path, device, cmd_list, state, out_staging);
        } else {
            return volume_benchmark_base::load_xfer_func(
                path, device, cmd_list, state, out_staging);
        }

    } catch (...) {
        log::instance().write_line(log_level::debug, "Creating linear transfer "
            "function as fallback solution.");
        std::vector<std::uint8_t> data(256 * 4);
        for (std::size_t i = 0; i < 256; ++i) {
            data[i * 4 + 0] = static_cast<std::uint8_t>(i);
            data[i * 4 + 1] = static_cast<std::uint8_t>(i);
            data[i * 4 + 2] = static_cast<std::uint8_t>(i);
            data[i * 4 + 3] = static_cast<std::uint8_t>(i);
        }

        return volume_benchmark_base::load_xfer_func(
            data, device, cmd_list, state, out_staging);
    }
}


/*
 * trrojan::d3d12::volume_benchmark_base::volume_benchmark_base
 */
trrojan::d3d12::volume_benchmark_base::volume_benchmark_base(
        const std::string& name) : benchmark_base(name) {
    this->_default_configs.add_factor(factor::from_manifestations(
        factor_ert_threshold, 0.0f));
    this->_default_configs.add_factor(factor::from_manifestations(
        factor_frame, static_cast<frame_type>(0)));
    this->_default_configs.add_factor(factor::from_manifestations(
        factor_fovy_deg, 60.0f));
    this->_default_configs.add_factor(factor::from_manifestations(
        factor_gpu_counter_iterations, static_cast<unsigned int>(7)));
    this->_default_configs.add_factor(factor::from_manifestations(
        factor_step_size, static_cast<step_size_type>(1)));
    this->_default_configs.add_factor(factor::from_manifestations(
        factor_max_steps, static_cast<unsigned int>(0)));
    this->_default_configs.add_factor(factor::from_manifestations(
        factor_min_prewarms, static_cast<unsigned int>(4)));
    this->_default_configs.add_factor(factor::from_manifestations(
        factor_min_wall_time, static_cast<unsigned int>(1000)));

    this->add_default_manoeuvre();
}


/*
 * trrojan::d3d12::volume_benchmark_base::on_device_switch
 */
void trrojan::d3d12::volume_benchmark_base::on_device_switch(device& device) {
    benchmark_base::on_device_switch(device);
    this->_tex_volume = nullptr;
    this->_tex_xfer_func = nullptr;
}


/*
 * trrojan::d3d12::volume_benchmark_base::on_run
 */
trrojan::result trrojan::d3d12::volume_benchmark_base::on_run(
        d3d12::device& device,
        const configuration& config,
        power_collector::pointer& power_collector,
        const std::vector<std::string>& changed) {
    static const auto res_target_state
        = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
        | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        //D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE

    // Clear all resource that have been invalidated by the change of factors.
    if (contains_any(changed, factor_device, factor_data_set, factor_frame)) {
        this->_tex_volume = nullptr;
    }
    if (contains_any(changed, factor_device, factor_xfer_func)) {
        this->_tex_xfer_func = nullptr;
    }

    // Create all resources managed by the base class that are invalid.
    {
        auto cmd_list = this->create_graphics_command_list_for(
            this->_tex_volume,
            this->_tex_xfer_func);
        // The following variable keeps the staging buffer alive until the
        // command list has completed processing. This is required or the
        // whole stuff crashes.
        std::array<winrt::com_ptr<ID3D12Resource>, 2> staging_buffers;

        if (this->_tex_volume == nullptr) {
            this->_tex_volume = this->load_volume(config, device,
                cmd_list.get(), res_target_state, this->_volume_info,
                staging_buffers.front());
            this->calc_bounding_box(this->_volume_bbox.front(),
                this->_volume_bbox.back());
        }

        if (this->_tex_xfer_func == nullptr) {
            this->_tex_xfer_func = this->load_xfer_func(config, device,
                cmd_list.get(), res_target_state, staging_buffers.back());
        }

        if (cmd_list != nullptr) {
            device.close_and_execute_command_list(cmd_list);
            device.wait_for_gpu();
        }
    }

    // Update the camera from the configuration.
    this->set_aspect_from_viewport(this->_camera);
    this->_camera.set_fovy(config.get<float>(factor_fovy_deg));
    graphics_benchmark_base::apply_manoeuvre(this->_camera, config,
        this->_volume_bbox.front(), this->_volume_bbox.back());
    trrojan::set_clipping_planes(this->_camera, this->_volume_bbox);

    return trrojan::result();
}


/*
 * trrojan::d3d12::volume_benchmark_base::set_textures
 */
void trrojan::d3d12::volume_benchmark_base::set_textures(
        const D3D12_CPU_DESCRIPTOR_HANDLE handle_volume,
        const D3D12_CPU_DESCRIPTOR_HANDLE handle_xfer_func) const {
    assert(this->_tex_volume != nullptr);
    assert(this->_tex_xfer_func != nullptr);
    auto device = get_device(this->_tex_volume);
    device->CreateShaderResourceView(this->_tex_volume.get(), nullptr,
        handle_volume);
    device->CreateShaderResourceView(this->_tex_xfer_func.get(), nullptr,
        handle_xfer_func);
}
