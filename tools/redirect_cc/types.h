/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_TOOLS_REDIRECT_CC_TYPES_H_
#define BRAVE_TOOLS_REDIRECT_CC_TYPES_H_

#include <string>
#include <string_view>

#if defined(_WIN32)
using FilePathChar = wchar_t;
#define FILE_PATH_LITERAL(x) L##x
#else  // defined(_WIN32)
using FilePathChar = char;
#define FILE_PATH_LITERAL(x) x
#endif  // defined(_WIN32)

using FilePathString = std::basic_string<FilePathChar>;
using FilePathStringView = std::basic_string_view<FilePathChar>;

#endif  // BRAVE_TOOLS_REDIRECT_CC_TYPES_H_
