// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include "../include/librealsense2/rs.h"
#include "../include/librealsense2/rs.hpp"

#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <thread>
#include <map>
//////////////////////////////
// Basic Data Types         //
//////////////////////////////

struct float3 { float x, y, z; };
struct float2 { float x, y; };

struct rect
{
    float x, y;
    float w, h;

    // Create new rect within original boundaries with give aspect ration
    rect adjust_ratio(float2 size) const
    {
        auto H = static_cast<float>(h), W = static_cast<float>(h) * size.x / size.y;
        if (W > w)
        {
            auto scale = w / W;
            W *= scale;
            H *= scale;
        }

        return{ x + (w - W) / 2, y + (h - H) / 2, W, H };
    }
};

class viewer_window : public rs2::processing_block
{
public:
    viewer_window(int window_width, int window_height, std::string window_title = "RealSense Viewer") :
        _input_queue(1),
        _window_width(window_width),
        _window_height(window_height),
        _window_title(window_title),
        processing_block([this](rs2::frame f, rs2::frame_source& s)
    {
        //f.keep();
        _input_queue.enqueue(f);
        s.frame_ready(f);
    })
    {
        _render_thread = std::thread(&viewer_window::render_thread, this);
    }

    ~viewer_window()
    {
        if(_render_thread.joinable())
            _render_thread.join();
        _last_frames.clear();
        glfwTerminate();
    }

    operator bool()
    {
        return !glfwWindowShouldClose(_window.get());
    }

private:
    int _window_width;
    int _window_height;
    std::string _window_title;

    std::shared_ptr<GLFWwindow> _window;
    GLuint _frame_buffer_name = 0;

    rs2::frame_queue _input_queue;
    std::thread _render_thread;

    std::map<std::pair<rs2_stream, int>, rect> _view_ports;
    std::map<std::pair<rs2_stream, int>, float2> _frame_size;
    std::map<std::pair<rs2_stream, int>, rs2::frame> _last_frames;

    std::shared_ptr<GLFWwindow> create_gl_window()
    {
        glfwInit();
        auto w = glfwCreateWindow(_window_width, _window_height, _window_title.c_str(), nullptr, nullptr);
        if (!w)
            throw std::runtime_error("Could not open OpenGL window, please check your graphic drivers or use the textual SDK tools");
        return std::shared_ptr<GLFWwindow>(w, [](GLFWwindow * w) 
        {
            glfwDestroyWindow(w); 
        });
    }

    bool try_add_frame(const rs2::frame& f)
    {
        if (!can_render(f)) 
            return false;
        auto p = f.get_profile();
        _last_frames[{p.stream_type(), p.stream_index()}] = f;
        update_viewports_grid(f);
        return true;
    }

    bool try_handle_frame(const rs2::frame& f)
    {
        if (!f)
            return false;
        bool rv = false;
        if (f.is<rs2::frameset>())
        {
            auto set = f.as<rs2::frameset>();
            set.foreach([&](rs2::frame f)
            {
                if (try_add_frame(f))
                    rv = true;
            });
        }
        else
            return try_add_frame(f);
        return rv;
    }

    void render_thread()
    {
        _window = create_gl_window();
        glGenTextures(1, &_frame_buffer_name);

        while (!glfwWindowShouldClose(_window.get()))
        {
            auto f = _input_queue.wait_for_frame();

            try_handle_frame(f);

            glfwMakeContextCurrent(_window.get());
            glfwPollEvents();

            for (auto& r : _last_frames)
            {
                render(r.second);
            }

            glfwSwapBuffers(_window.get());
            glFlush();
        }
    }

    void update_viewports_grid(const rs2::frame& f)
    {
        auto p = f.get_profile();
        auto it = _frame_size.find({ p.stream_type(), p.stream_index() });
        auto vf = f.as<rs2::video_frame>();
        if (it == _frame_size.end())
            _frame_size[{p.stream_type(), p.stream_index()}] = { (float)vf.get_width(), (float)vf.get_height() };

        calc_grid();

        glViewport(0, 0, _window_width, _window_height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void render(const rs2::frame& f)
    {
        auto p = f.get_profile();
        auto r = _view_ports[{p.stream_type(), p.stream_index()}];
        auto& vf = f.as<rs2::video_frame>();

        glViewport(r.x, r.y, r.w, r.h);

        glLoadIdentity();
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glOrtho(0, r.w, r.h, 0, -1, +1);

        auto format = f.get_profile().format();
        switch (format)
        {
        case RS2_FORMAT_RGB8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, vf.get_width(), vf.get_height(), 0, GL_RGB, GL_UNSIGNED_BYTE, f.get_data());
            break;
        case RS2_FORMAT_RGBA8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vf.get_width(), vf.get_height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, f.get_data());
            break;
        case RS2_FORMAT_Y8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, vf.get_width(), vf.get_height(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, f.get_data());
            break;
        default:
            throw std::runtime_error("The requested format is not supported by this demo!");
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
        glEnable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(0, 0);
        glTexCoord2f(0, 1); glVertex2f(0, r.h);
        glTexCoord2f(1, 1); glVertex2f(r.w, r.h);
        glTexCoord2f(1, 0); glVertex2f(r.w, 0);
        glEnd();
        glDisable(GL_TEXTURE_2D);
        glPopMatrix();
    }

    rect calc_cell_size(float2 window, int streams)
    {
        if (window.x <= 0 || window.y <= 0 || streams <= 0)
            throw std::runtime_error("invalid window configuration request, failed to calculate window grid");
        float ratio = window.x / window.y;
        auto x = sqrt(ratio * (float)streams);
        auto y = (float)streams / x;
        auto w = round(x);
        auto h = round(y);
        if (w == 0 || h == 0)
            throw std::runtime_error("invalid window configuration request, failed to calculate window grid");
        while (w*h > streams)
            h > w ? h-- : w--;
        while (w*h < streams)
            h > w ? w++ : h++;
        auto new_w = round(window.x / w);
        auto new_h = round(window.y / h);
        // column count, line count, cell width cell height
        return rect{ static_cast<float>(w), static_cast<float>(h), static_cast<float>(new_w), static_cast<float>(new_h) };
    }

    void calc_grid()
    {
        glfwGetWindowSize(_window.get(), &_window_width, &_window_height);
        auto grid = calc_cell_size({(float)_window_width, (float)_window_height}, _frame_size.size());

        int index = 0;
        int curr_line = -1;
        for (auto f : _frame_size)
        {
            auto mod = index % (int)grid.x;
            auto fw = f.second.x;
            auto fh = f.second.y;

            float cell_x_postion = (float)(mod * grid.w);
            if (mod == 0) curr_line++;
            float cell_y_position = curr_line * grid.h;

            auto r = rect{ cell_x_postion, cell_y_position, grid.w, grid.h };
            _view_ports[f.first] = r.adjust_ratio(float2{ fw, fh });
            index++;
        }
    }

    bool can_render(const rs2::frame& f) const
    {
        auto format = f.get_profile().format();
        switch (format)
        {
        case RS2_FORMAT_RGB8:
        case RS2_FORMAT_RGBA8:
        case RS2_FORMAT_Y8:
            return true;
        default:
            return false;
        }
    }
};
