/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// This file contains code that used to be upstream and had to be restored in
// Brave to support delta updates on Windows until we are on Omaha 4. See:
// github.com/brave/brave-core/pull/31937

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(crbug.com/40285824): Remove this and convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include <chrome/installer/mini_installer/mini_string.cc>

namespace mini_installer {

const wchar_t* SearchStringI(const wchar_t* source, const wchar_t* find) {
  if (!find || find[0] == L'\0') {
    return source;
  }

  const wchar_t* scan = source;
  while (*scan) {
    const wchar_t* s = scan;
    const wchar_t* f = find;

    while (*s && *f && EqualASCIICharI(*s, *f)) {
      ++s, ++f;
    }

    if (!*f) {
      return scan;
    }

    ++scan;
  }

  return nullptr;
}

bool FindTagInStr(const wchar_t* str,
                  const wchar_t* tag,
                  const wchar_t** position) {
  int tag_length = ::lstrlen(tag);
  const wchar_t* scan = str;
  for (const wchar_t* tag_start = SearchStringI(scan, tag);
       tag_start != nullptr; tag_start = SearchStringI(scan, tag)) {
    scan = tag_start + tag_length;
    if (*scan == L'-' || *scan == L'\0') {
      if (position != nullptr) {
        *position = tag_start;
      }
      return true;
    }
  }
  return false;
}

}  // namespace mini_installer
