/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/http/transport_security_state.h"

#include "build/build_config.h"

#if !defined(OS_ANDROID) && !defined(OS_IOS)

#define BRAVE_ENABLE_STATIC_PINS \
  enable_static_pins_ = true;    \
  enable_static_expect_ct_ = true;

#else

// Leave static pins disabled on Android and iOS, like upstream.
#define BRAVE_ENABLE_STATIC_PINS \
  {}

#endif

#include "../../../../net/http/transport_security_state.cc"
