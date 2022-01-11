/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/tools/redirect_cc/logging.h"

#if defined(_WIN32)
#include <Windows.h>
#endif  // defined(_WIN32)

#include <iostream>

#include "brave/tools/redirect_cc/os_utils.h"

namespace logging {

bool IsVerbose() {
  static int is_verbose = -1;
  if (is_verbose == -1) {
    FilePathString verbose_var;
    if (os_utils::GetEnvVar(FILE_PATH_LITERAL("REDIRECT_CC_VERBOSE"),
                            &verbose_var)) {
      is_verbose = verbose_var == FILE_PATH_LITERAL("1") ? 1 : 0;
    } else {
      is_verbose = 0;
    }
  }
  return is_verbose == 1;
}

LogMessage::LogMessage(const char* file, int line) {
  std::string_view filename(file);
  size_t last_slash_pos = filename.find_last_of("\\/");
  if (last_slash_pos != std::string_view::npos)
    filename.remove_prefix(last_slash_pos + 1);

  stream() << "[" << filename << "(" << line << ")] ";
}

LogMessage::~LogMessage() {
  std::cerr << stream_.str();
}

}  // namespace logging

#if defined(_WIN32)
std::string SysWideToMultiByte(const std::wstring_view& wide,
                               uint32_t code_page) {
  int wide_length = static_cast<int>(wide.length());
  if (wide_length == 0)
    return std::string();

  // Compute the length of the buffer we'll need.
  int charcount = WideCharToMultiByte(code_page, 0, wide.data(), wide_length,
                                      NULL, 0, NULL, NULL);
  if (charcount == 0)
    return std::string();

  std::string mb;
  mb.resize(charcount);
  WideCharToMultiByte(code_page, 0, wide.data(), wide_length, &mb[0], charcount,
                      NULL, NULL);

  return mb;
}

std::ostream& operator<<(std::ostream& o, const std::wstring& str) {
  o << SysWideToMultiByte(str, CP_ACP);
  return o;
}

std::ostream& operator<<(std::ostream& o, std::wstring_view piece) {
  o << SysWideToMultiByte(piece, CP_ACP);
  return o;
}
#endif  // defined(_WIN32)
