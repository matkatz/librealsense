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
#include <mutex>

#define M_PI 3.14159265359
#define DEFAULT_SUB_WINDOW_SIZE 200
#include "../third-party/stb_easy_font.h"

inline void draw_text(int x, int y, const char * text)
{
    char buffer[60000]; // ~300 chars
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, 4 * stb_easy_font_print((float)x, (float)(y - 7), (char *)text, nullptr, buffer, sizeof(buffer)));
    glDisableClientState(GL_VERTEX_ARRAY);
}

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

class viewer : public rs2::processing_block
{
public:
    viewer(int window_width, int window_height, std::string window_title = "RealSense Viewer") :
        _input_queue(1),
        _window_width(window_width),
        _window_height(window_height),
        _window_title(window_title),
        processing_block([this](rs2::frame& f, const rs2::frame_source& s)
    {
        _input_queue.enqueue(f);
        s.frame_ready(f);
    })
    {
        _render_thread = std::thread(&viewer::render_thread, this);
    }

    ~viewer()
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
        _frame_size[{RS2_STREAM_GYRO, 0}] = { 640,640 };
        _frame_size[{RS2_STREAM_ACCEL, 0}] = { 640,640 };

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
        float a = 0.1;

        while (!glfwWindowShouldClose(_window.get()))
        {
            glfwMakeContextCurrent(_window.get());
            glfwPollEvents();

            rs2::frame f;
            if (!_input_queue.poll_for_frame(&f))
                continue;

            try_handle_frame(f);


            for (auto&& r : _last_frames)
            {
                if(r.second.is<rs2::video_frame>())
                    render_video_frame(r.second);
                if (r.second.is<rs2::motion_frame>())
                    render_motion_frame(r.second);
            }
            a += 0.1;
            draw_motion_data(sin(a), cos(a), sin(a), _view_ports[{RS2_STREAM_GYRO, 0}], "GYRO");
            draw_motion_data(cos(a), sin(a), cos(a), _view_ports[{RS2_STREAM_ACCEL, 0}], "ACCEL");

            glfwSwapBuffers(_window.get());
            glFlush();
        }
    }

    void update_viewports_grid(const rs2::frame& f)
    {
        auto p = f.get_profile();
        auto it = _frame_size.find({ p.stream_type(), p.stream_index() });
        if (it == _frame_size.end())
        {
            if (auto vf = f.as<rs2::video_frame>())
                _frame_size[{p.stream_type(), p.stream_index()}] = { (float)vf.get_width(), (float)vf.get_height() };
            else
                _frame_size[{p.stream_type(), p.stream_index()}] = { DEFAULT_SUB_WINDOW_SIZE, DEFAULT_SUB_WINDOW_SIZE };
        }

        calc_grid();

        glViewport(0, 0, _window_width, _window_height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void render_motion_frame(const rs2::motion_frame& f)
    {
        auto p = f.get_profile();
        auto r = _view_ports[{p.stream_type(), p.stream_index()}];
        glViewport(r.x, r.y, r.w, r.h);
    }

    void render_video_frame(const rs2::video_frame& f)
    {
        auto p = f.get_profile();
        auto r = _view_ports[{p.stream_type(), p.stream_index()}];

        glViewport(r.x, r.y, r.w, r.h);

        glLoadIdentity();
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glOrtho(0, r.w, r.h, 0, -1, +1);

        auto format = f.get_profile().format();
        switch (format)
        {
        case RS2_FORMAT_RGB8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, f.get_width(), f.get_height(), 0, GL_RGB, GL_UNSIGNED_BYTE, f.get_data());
            break;
        case RS2_FORMAT_RGBA8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, f.get_width(), f.get_height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, f.get_data());
            break;
        case RS2_FORMAT_Y8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, f.get_width(), f.get_height(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, f.get_data());
            break;
        default:
            throw std::runtime_error("The requested format is not supported by this demo!");
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, _frame_buffer_name);
        glEnable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(0, 0);
        glTexCoord2f(0, 1); glVertex2f(0, r.h);
        glTexCoord2f(1, 1); glVertex2f(r.w, r.h);
        glTexCoord2f(1, 0); glVertex2f(r.w, 0);
        glEnd();
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        draw_text(20, r.h-10, p.stream_name().c_str());
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
        case RS2_FORMAT_MOTION_XYZ32F:
            return true;
        default:
            return false;
        }
    }

    static void  draw_axes(float axis_size = 1.f, float axisWidth = 4.f)
    {

        // Triangles For X axis
        glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(axis_size * 1.1f, 0.f, 0.f);
        glVertex3f(axis_size, -axis_size * 0.05f, 0.f);
        glVertex3f(axis_size, axis_size * 0.05f, 0.f);
        glVertex3f(axis_size * 1.1f, 0.f, 0.f);
        glVertex3f(axis_size, 0.f, -axis_size * 0.05f);
        glVertex3f(axis_size, 0.f, axis_size * 0.05f);
        glEnd();

        // Triangles For Y axis
        glBegin(GL_TRIANGLES);
        glColor3f(0.f, 1.f, 0.f);
        glVertex3f(0.f, axis_size * 1.1f, 0.0f);
        glVertex3f(0.f, axis_size, 0.05f * axis_size);
        glVertex3f(0.f, axis_size, -0.05f * axis_size);
        glVertex3f(0.f, axis_size * 1.1f, 0.0f);
        glVertex3f(0.05f * axis_size, axis_size, 0.f);
        glVertex3f(-0.05f * axis_size, axis_size, 0.f);
        glEnd();

        // Triangles For Z axis
        glBegin(GL_TRIANGLES);
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 1.1f * axis_size);
        glVertex3f(0.0f, 0.05f * axis_size, 1.0f * axis_size);
        glVertex3f(0.0f, -0.05f * axis_size, 1.0f * axis_size);
        glVertex3f(0.0f, 0.0f, 1.1f * axis_size);
        glVertex3f(0.05f * axis_size, 0.f, 1.0f * axis_size);
        glVertex3f(-0.05f * axis_size, 0.f, 1.0f * axis_size);
        glEnd();

        glLineWidth(axisWidth);

        // Drawing Axis
        glBegin(GL_LINES);
        // X axis - Red
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(axis_size, 0.0f, 0.0f);

        // Y axis - Green
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, axis_size, 0.0f);

        // Z axis - Blue
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, axis_size);
        glEnd();
    }

    // intensity is grey intensity
    static void draw_circle(float xx, float xy, float xz, float yx, float yy, float yz, float radius = 1.1, float3 center = { 0.0, 0.0, 0.0 }, float intensity = 0.5f)
    {
        const auto N = 50;
        glColor3f(intensity, intensity, intensity);
        glLineWidth(2);
        glBegin(GL_LINE_STRIP);

        for (int i = 0; i <= N; i++)
        {
            const double theta = (2 * M_PI / N) * i;
            const auto cost = static_cast<float>(cos(theta));
            const auto sint = static_cast<float>(sin(theta));
            glVertex3f(
                center.x + radius * (xx * cost + yx * sint),
                center.y + radius * (xy * cost + yy * sint),
                center.z + radius * (xz * cost + yz * sint)
            );
        }

        glEnd();
    }

    void draw_motion_data(float x, float y, float z, rect r, std::string str)
    {
        draw_text(20, r.h - 10, str.c_str());

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();

        glViewport(r.x, r.y, r.w, r.h);
        //glClearColor(0, 0, 0, 1);
        //glClear(GL_COLOR_BUFFER_BIT);
        draw_text(20, r.h - 10, str.c_str());

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        glOrtho(-2.8, 2.8, -2.4, 2.4, -7, 7);

        glRotatef(25, 1.0f, 0.0f, 0.0f);

        glTranslatef(0, -0.33f, -1.f);

        float norm = std::sqrt(x*x + y * y + z * z);

        glRotatef(-135, 0.0f, 1.0f, 0.0f);

        draw_axes();

        draw_circle(1, 0, 0, 0, 1, 0);
        draw_circle(0, 1, 0, 0, 0, 1);
        draw_circle(1, 0, 0, 0, 0, 1);

        const auto canvas_size = 230;
        const auto vec_threshold = 0.01f;
        if (norm < vec_threshold)
        {
            const auto radius = 0.05;
            static const int circle_points = 100;
            static const float angle = 2.0f * 3.1416f / circle_points;

            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_POLYGON);
            double angle1 = 0.0;
            glVertex2d(radius * cos(0.0), radius * sin(0.0));
            int i;
            for (i = 0; i < circle_points; i++)
            {
                glVertex2d(radius * cos(angle1), radius *sin(angle1));
                angle1 += angle;
            }
            glEnd();
        }
        else
        {
            auto vectorWidth = 5.f;
            glLineWidth(vectorWidth);
            glBegin(GL_LINES);
            glColor3f(1.0f, 1.0f, 1.0f);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(x / norm, y / norm, z / norm);
            glEnd();

            // Save model and projection matrix for later
            GLfloat model[16];
            glGetFloatv(GL_MODELVIEW_MATRIX, model);
            GLfloat proj[16];
            glGetFloatv(GL_PROJECTION_MATRIX, proj);

            glLoadIdentity();
            glOrtho(-canvas_size, canvas_size, -canvas_size, canvas_size, -1, +1);

            std::ostringstream s1;
            const auto precision = 3;

            //s1 << "(" << std::fixed << std::setprecision(precision) << x << "," << std::fixed << std::setprecision(precision) << y << "," << std::fixed << std::setprecision(precision) << z << ")";
            //print_text_in_3d(x, y, z, s1.str().c_str(), false, model, proj, 1 / norm);

            std::ostringstream s2;
            //s2 << std::setprecision(precision) << norm;
            //print_text_in_3d(x / 2, y / 2, z / 2, s2.str().c_str(), true, model, proj, 1 / norm);
        }
        draw_text(20, r.h + 10, str.c_str());

        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, r.x, r.y, r.w, r.h, 0);
        draw_text(20, r.h - 10, str.c_str());

        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        draw_text(20, r.h - 10, str.c_str());

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        draw_text(20, r.h + 10, str.c_str());
    }
};
