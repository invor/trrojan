﻿// <copyright file="d2d_overlay.cpp" company="Visualisierungsinstitut der Universität Stuttgart">
// Copyright © 2022 - 2024 Visualisierungsinstitut der Universität Stuttgart.
// Licensed under the MIT licence. See LICENCE.txt file in the project root for full licence information.
// </copyright>
// <author>Christoph Müller</author>
// <author>Michael Becher</author>

#include "trrojan/d3d12/d2d_overlay.h"

#include <algorithm>

#include "trrojan/com_error_category.h"

#include "trrojan/d3d12/plugin.h"
#include "trrojan/d3d12/utilities.h"


/*
 * trrojan::d3d11::d2d_overlay::get_font
 */
winrt::com_ptr<IDWriteFont> trrojan::d3d12::d2d_overlay::get_font(
    IDWriteTextFormat* format) {
    BOOL exists = FALSE;
    winrt::com_ptr<IDWriteFontFamily> family;
    winrt::com_ptr<IDWriteFontCollection> fc;
    UINT32 index = 0;
    std::vector<wchar_t> name;
    winrt::com_ptr<IDWriteFont> retval;

    if (format == nullptr) {
        throw std::system_error(E_POINTER, com_category());
    }

    {
        auto cntBuffer = format->GetFontFamilyNameLength() + 1;
        name.resize(cntBuffer);
        auto hr = format->GetFontFamilyName(name.data(), cntBuffer);
        if (FAILED(hr)) {
            throw std::system_error(hr, com_category());
        }
    }

    {
        auto hr = format->GetFontCollection(fc.put());
        if (FAILED(hr)) {
            throw std::system_error(hr, com_category());
        }
    }

    {
        auto hr = fc->FindFamilyName(name.data(), &index, &exists);
        if (FAILED(hr)) {
            throw std::system_error(hr, com_category());
        }
        if (!exists) {
            throw std::system_error(E_FAIL, com_category());
        }
    }

    {
        auto hr = fc->GetFontFamily(index, family.put());
        if (FAILED(hr)) {
            throw std::system_error(E_FAIL, com_category());
        }
    }

    {
        auto hr = family->GetFirstMatchingFont(format->GetFontWeight(),
            format->GetFontStretch(), format->GetFontStyle(), retval.put());
        if (FAILED(hr)) {
            throw std::system_error(E_FAIL, com_category());
        }
    }

    return retval;
}


/*
 * trrojan::d3d11::d2d_overlay::d2d_overlay
 */
trrojan::d3d12::d2d_overlay::d2d_overlay(
        winrt::com_ptr<ID3D12Device> device,
        winrt::com_ptr<ID3D12CommandQueue> command_queue,
        winrt::com_ptr<IDXGISwapChain3> swap_chain,
        UINT frame_count)
    :  _d3d12_device(device),
        _d3d12_command_queue(command_queue),
        _swap_chain(swap_chain),
        _frame_count(frame_count) {
    assert(this->_d3d12_device != nullptr);
    assert(this->_d3d12_command_queue != nullptr);
    assert(this->_swap_chain != nullptr);
    this->on_resized();
}


/*
 * trrojan::d3d11::d2d_overlay::begin_draw
 */
void trrojan::d3d12::d2d_overlay::begin_draw(UINT frame_index) {
    assert(this->_d2d_context != nullptr);
    assert(this->_drawing_state_block != nullptr);
    assert(this->_d3d11on12_device != nullptr);

    if (this->_d2d_context == nullptr) {
        throw std::system_error(E_NOT_VALID_STATE, com_category());
    }

    this->_current_frame = frame_index;

    this->_d2d_context->SaveDrawingState(this->_drawing_state_block.get());

    // Acquire our wrapped render target resource for the current back buffer.
    auto buffers = unsmart(this->_wrapped_back_buffers[this->_current_frame]);
    this->_d3d11on12_device->AcquireWrappedResources(buffers.data(), 1);

    this->_d2d_context->SetTarget(this->_d2d_render_targets[frame_index].get());

    this->_d2d_context->BeginDraw();
}


/*
 * trrojan::d3d11::d2d_overlay::create_brush
 */
winrt::com_ptr<ID2D1Brush> trrojan::d3d12::d2d_overlay::create_brush(
        const D2D1::ColorF& colour) {
    if (this->_d2d_context == nullptr) {
        throw std::system_error(E_NOT_VALID_STATE, com_category());
    }

    winrt::com_ptr<ID2D1Brush> retval;
    auto hr = this->_d2d_context->CreateSolidColorBrush(colour,
        reinterpret_cast<ID2D1SolidColorBrush**>(retval.put()));
    if (FAILED(hr)) {
        throw std::system_error(hr, com_category());
    }

    return retval;
}


/*
 * trrojan::d3d11::d2d_overlay::create_text_format
 */
winrt::com_ptr<IDWriteTextFormat> trrojan::d3d12::d2d_overlay::create_text_format(
        const wchar_t* font_family,
        const float font_size,
        const DWRITE_FONT_WEIGHT font_weight,
        const DWRITE_FONT_STYLE font_style,
        const DWRITE_FONT_STRETCH font_stretch,
        const wchar_t* locale_name) {
    assert(this->_dwrite_factory != nullptr);

    std::vector<wchar_t> locale;    // The actual locale name.

    // If no locale is given, retrieve the name of the current user locale
    // and use this one. Otherwise, use the caller-defined locale name.
    if (locale_name == NULL) {
        auto locale_len = ::GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT,
            LOCALE_SNAME, NULL, 0);
        if (locale_len != 0) {
            locale.resize(locale_len);
            locale_len = ::GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT,
                LOCALE_SNAME, locale.data(), locale_len);
        }
        if (locale_len == 0) {
            throw std::system_error(::GetLastError(), std::system_category());
        }

    }
    else {
        auto locale_len = ::wcslen(locale_name) + 1;
        locale.reserve(locale_len);
        std::copy(locale_name, locale_name + locale_len, locale.begin());
    }

    winrt::com_ptr<IDWriteTextFormat> retval;
    auto hr = this->_dwrite_factory->CreateTextFormat(font_family, NULL,
        font_weight, font_style, font_stretch, font_size, locale.data(),
        retval.put());
    if (FAILED(hr)) {
        throw std::system_error(hr, com_category());

    }

    return retval;
}


/*
 * trrojan::d3d11::d2d_overlay::draw_text
 */
void trrojan::d3d12::d2d_overlay::draw_text(const wchar_t* text,
    IDWriteTextFormat* format, ID2D1Brush* brush,
    const D2D1_RECT_F* layout_rect) {
    assert(this->_d2d_context != nullptr);
    assert(this->_d2d_render_targets[this->_current_frame] != nullptr);

    auto len = (text != nullptr) ? ::wcslen(text) : 0;
    D2D1_RECT_F rect;

    // Determine the layout rectangle as the whole render target if no
    // user-defined one was specified.
    if (layout_rect != nullptr) {
        rect = *layout_rect;
    }
    else {
        D2D1_SIZE_F size = this->_d2d_render_targets[this->_current_frame]->GetSize();
        rect = D2D1::RectF(0.0f, 0.0f, size.width, size.height);
    }

    this->_d2d_context->DrawText(text, static_cast<UINT32>(len), format, rect,
        brush);
}


/*
 * trrojan::d3d11::d2d_overlay::end_draw
 */
void trrojan::d3d12::d2d_overlay::end_draw(void) {
    assert(this->_d2d_context != nullptr);
    assert(this->_d3d11on12_device != nullptr);
    assert(this->_d3d11_device_context != nullptr);

    auto hr = _d2d_context->EndDraw();
    if (FAILED(hr) && (hr != D2DERR_RECREATE_TARGET)) {
        throw std::system_error(hr, com_category());
    }

    // Release our wrapped render target resource. Releasing 
    // transitions the back buffer resource to the state specified
    // as the OutState when the wrapped resource was created.
    auto buffers = unsmart(this->_wrapped_back_buffers[this->_current_frame]);
    this->_d3d11on12_device->ReleaseWrappedResources(buffers.data(), 1);

    // Flush to submit the 11 command list to the shared command queue
    this->_d3d11_device_context->Flush();

    this->_d2d_context->RestoreDrawingState(this->_drawing_state_block.get());
}


/*
 * trrojan::d3d11::d2d_overlay::on_resize
 */
void trrojan::d3d12::d2d_overlay::on_resize(void) {
    this->release_target_dependent_resources();
}


/*
 * trrojan::d3d11::d2d_overlay::on_resized
 */
void trrojan::d3d12::d2d_overlay::on_resized(void) {
    this->create_target_independent_resources();
    this->create_target_dependent_resources();
}


/*
 * trrojan::d3d11::d2d_overlay::create_target_dependent_resources
 */
void trrojan::d3d12::d2d_overlay::create_target_dependent_resources(
    IDXGISurface* surface, int i) {
    assert(this->_d2d_context != nullptr);
    assert(surface != nullptr);

    {
        auto pixel_format = D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN,
            D2D1_ALPHA_MODE_PREMULTIPLIED);
        auto bmp_props = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            pixel_format);

        // We force the target to be nullpt here, because during initial resize
        // of the window, the swap chain resize process is not involved.
        this->_d2d_render_targets[i] = nullptr;
        auto hr = this->_d2d_context->CreateBitmapFromDxgiSurface(
            surface, &bmp_props, this->_d2d_render_targets[i].put());
        if (FAILED(hr)) {
            throw std::system_error(hr, com_category());
        }
    }

    this->_d2d_context->SetTextAntialiasMode(
        D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
}


/*
 * trrojan::d3d11::d2d_overlay::create_target_dependent_resources
 */
void trrojan::d3d12::d2d_overlay::create_target_dependent_resources(void) {
    assert(this->_d2d_context != nullptr);
    assert(this->_d2d_device != nullptr);
    assert(this->_d2d_factory != nullptr);
    assert(this->_d3d11on12_device != nullptr);
    assert(this->_swap_chain != nullptr);

    this->_render_targets.clear();
    this->_render_targets.resize(this->_frame_count);
    this->_wrapped_back_buffers.clear();
    this->_wrapped_back_buffers.resize(this->_frame_count);
    this->_d2d_render_targets.clear();
    this->_d2d_render_targets.resize(this->_frame_count);

    for (UINT i = 0; i < this->_frame_count; ++i) {
        auto hr = this->_swap_chain->GetBuffer(
            i, 
            IID_ID3D12Resource, 
            reinterpret_cast<void**>(&this->_render_targets[i])
        );
        if (FAILED(hr)) {
            throw std::system_error(E_POINTER, com_category());
        }

        d3d12::set_debug_object_name(this->_render_targets[i],
            "RenderTarget {}", i);


        // Create a wrapped 11On12 resource of this back buffer. Since we are 
        // rendering all D3D12 content first and then all D2D content, we specify 
        // the In resource state as RENDER_TARGET - because D3D12 will have last 
        // used it in this state - and the Out resource state as PRESENT. When 
        // ReleaseWrappedResources() is called on the 11On12 device, the resource 
        // will be transitioned to the PRESENT state.
        D3D11_RESOURCE_FLAGS d3d11_flags = { D3D11_BIND_RENDER_TARGET };
        hr = this->_d3d11on12_device->CreateWrappedResource(
            this->_render_targets[i].get(),
            &d3d11_flags,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT,
            __uuidof(ID3D11Resource),
            this->_wrapped_back_buffers[i].put_void());
        if (FAILED(hr)) {
            throw std::system_error(hr, com_category());
        }

        winrt::com_ptr<IDXGISurface> surface;
        hr = this->_wrapped_back_buffers[i]->QueryInterface(
            ::IID_IDXGISurface, 
            reinterpret_cast<void**>(&surface)
        );
        if (FAILED(hr)) {
            throw std::system_error(E_POINTER, com_category());
        }

        std::string name = "WrappedBuffer" + std::to_string(i);
        this->_wrapped_back_buffers[i]->SetPrivateData(WKPDID_D3DDebugObjectName, name.length(), name.c_str());

        this->create_target_dependent_resources(surface.get(), i);
    }
}


/*
 * trrojan::d3d11::d2d_overlay::create_target_independent_resources
 */
void trrojan::d3d12::d2d_overlay::create_target_independent_resources(void) {
    assert(this->_d2d_context == nullptr);
    assert(this->_d2d_device == nullptr);
    assert(this->_d2d_factory == nullptr);
    assert(this->_dwrite_factory == nullptr);
    assert(this->_drawing_state_block == nullptr);
    assert(this->_d3d11_device_context == nullptr);
    assert(this->_d3d11on12_device == nullptr);
    assert(this->_d3d12_device != nullptr);
    assert(this->_d3d12_command_queue != nullptr);
    // currently not used anyway
    //assert(this->_depth_stencil_state == nullptr);

    {
        auto hr = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED,
            ::IID_ID2D1Factory3,
            reinterpret_cast<void**>(&this->_d2d_factory));
        if (FAILED(hr)) {
            throw std::system_error(hr, com_category());
        }
    }

    {
        auto cmd_queue = this->_d3d12_command_queue.get();
        winrt::com_ptr<ID3D11Device> d3d11_device;
        UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        auto hr = D3D11On12CreateDevice(
            this->_d3d12_device.get(),
            deviceFlags,
            nullptr,
            0,
            reinterpret_cast<IUnknown**>(&cmd_queue),
            1,
            0,
            d3d11_device.put(),
            this->_d3d11_device_context.put(),
            nullptr);
        if (FAILED(hr)) {
            throw std::system_error(hr, com_category());
        }

        hr = d3d11_device->QueryInterface(
            __uuidof(ID3D11On12Device),
            reinterpret_cast<void**>(&this->_d3d11on12_device)
        );
        if (FAILED(hr)) {
            throw std::system_error(hr, com_category());
        }

        winrt::com_ptr<IDXGIDevice> idxgi_device;
        hr = this->_d3d11on12_device->QueryInterface(::IID_IDXGIDevice,
            reinterpret_cast<void**>(&idxgi_device));
        if (FAILED(hr)) {
            throw std::system_error(E_POINTER, com_category());
        }

        hr = this->_d2d_factory->CreateDevice(
            idxgi_device.get(), this->_d2d_device.put());
        if (FAILED(hr)) {
            throw std::system_error(hr, com_category());
        }
    }

    {
        auto hr = this->_d2d_device->CreateDeviceContext(
            D2D1_DEVICE_CONTEXT_OPTIONS_NONE, this->_d2d_context.put());
        if (FAILED(hr)) {
            throw std::system_error(hr, com_category());
        }
    }

    {
        auto hr = this->_d2d_factory->CreateDrawingStateBlock(
            this->_drawing_state_block.put());
        if (FAILED(hr)) {
            throw std::system_error(hr, com_category());
        }
    }

    {
        auto hr = ::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(&this->_dwrite_factory));
        if (FAILED(hr)) {
            throw std::system_error(hr, com_category());
        }
    }
}


/*
 * trrojan::d3d11::d2d_overlay::release_target_dependent_resources
 */
void trrojan::d3d12::d2d_overlay::release_target_dependent_resources(void) {
    this->_d2d_context->SetTarget(nullptr);
    this->_d2d_context = nullptr;
    for (auto rt : this->_render_targets) rt = nullptr;
    for (auto wbb : this->_wrapped_back_buffers) wbb = nullptr;
    for (auto d2drt : this->_d2d_render_targets) d2drt = nullptr;
    this->_render_targets.clear();
    this->_wrapped_back_buffers.clear();
    this->_d2d_render_targets.clear();
    this->_d3d11_device_context = nullptr;
    this->_d3d11on12_device = nullptr;
    this->_d2d_device = nullptr;

    // technically not needed, but release anyway
    // since we have to build some things that are based on the objects below
    this->_d2d_factory = nullptr;
    this->_drawing_state_block = nullptr;
    this->_dwrite_factory = nullptr;
}
