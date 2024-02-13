/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BUILD_BUILDFLAG_H_
#define BRAVE_CHROMIUM_SRC_BUILD_BUILDFLAG_H_

// This file is used in rc.exe which doesn't receive low priority additional
// global include to support #include "src/...".
// #include_next is used instead to include original upstream file.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-include-next"
#include_next "build/buildflag.h"
#pragma clang diagnostic pop

// Use IF_BUILDFLAG(FLAG_NAME, { some_code_here(); }) to generate a single
// conditional define.
//
// Before:
// #if BUILDFLAG(FLAG_NAME)
// #define BRAVE_FLAG_NAME_ADDITION some_code_here();
// #else  // BUILDFLAG(FLAG_NAME)
// #define BRAVE_FLAG_NAME_ADDITION
// #endif // BUILDFLAG(FLAG_NAME)
//
// After:
// #define BRAVE_FLAG_NAME_ADDITION \
//   IF_BUILDFLAG(FLAG_NAME, { some_code_here(); })
//
#define IF_BUILDFLAG_IMPL_0(...)
#define IF_BUILDFLAG_IMPL_1(...) __VA_ARGS__
#define IF_BUILDFLAG_IMPL_CAT(flag_value) \
  BUILDFLAG_CAT(IF_BUILDFLAG_IMPL_, flag_value)
#define IF_BUILDFLAG_IMPL(flag_value, ...) \
  IF_BUILDFLAG_IMPL_CAT flag_value(__VA_ARGS__)
#define IF_BUILDFLAG(flag, ...) \
  IF_BUILDFLAG_IMPL(BUILDFLAG_CAT(BUILDFLAG_INTERNAL_, flag)(), __VA_ARGS__)

#endif  // BRAVE_CHROMIUM_SRC_BUILD_BUILDFLAG_H_
