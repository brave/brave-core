/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/widevine/cdm/buildflags.h"

#if !BUILDFLAG(ENABLE_WIDEVINE)
// We need this value for DCHECK only.
inline constexpr char kWidevineKeySystem[] = "com.widevine.alpha";
#endif  // !BUILDFLAG(ENABLE_WIDEVINE)

#include "src/chrome/renderer/media/chrome_key_systems.cc"
