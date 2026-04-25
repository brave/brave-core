/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#if defined(ARCH_CPU_X86)
// x86 doesn't have enough RAM to allocate the file used in the VeryLarge test.
#define VeryLarge DISABLED_VeryLarge
#endif

#include <chrome/installer/util/unbuffered_file_writer_unittest.cc>

#if defined(ARCH_CPU_X86)
#undef VeryLarge
#endif
