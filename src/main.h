/*
MIT License

Copyright (c) [2025] [azula1A89]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <imgui.h>

using texture_t = std::pair<ImTextureID, ImVec2>;
extern ImFont*icon, *mdicon, *H1, *H2, *H3;
extern std::string choose_path();
extern texture_t load_texture_from_file(std::string& filepath);
extern texture_t load_texture_from_base64(std::string& base64);
extern std::unordered_map<std::string, std::shared_ptr<texture_t>> loaded_textures;//<filepath, texture>
extern std::unordered_map<ImTextureID, std::shared_ptr<std::string>> loaded_textures_base64;//<filepath, base64>
