#pragma once

#include <string>

std::string utf8_encode(const std::wstring& wstr);
std::wstring utf8_decode(const std::string& str);
