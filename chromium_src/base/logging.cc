/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/logging.h"

// Xcode's clang doesn't know about #pragma clang max_tokens_here
#if !defined(ANDROID) && defined(__APPLE__)
#include <TargetConditionals.h>
#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#define NACL_TC_REV 1
#endif
#endif
#include "src/base/logging.cc"
