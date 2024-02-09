/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_RENDERER_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_RENDERER_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_

#include "extensions/buildflags/buildflags.h"
#include "third_party/blink/public/platform/url_loader_throttle_provider.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/renderer/extension_throttle_manager.h"
#endif

#define SetOnline                                                    \
  SetOnlineUnused();                                                 \
                                                                     \
 protected:                                                          \
  static base::PassKey<URLLoaderThrottleProviderImpl> GetPassKey() { \
    return base::PassKey<URLLoaderThrottleProviderImpl>();           \
  }                                                                  \
                                                                     \
 public:                                                             \
  void SetOnline

#include "src/chrome/renderer/url_loader_throttle_provider_impl.h"  // IWYU pragma: export

#undef SetOnline

#endif  // BRAVE_CHROMIUM_SRC_CHROME_RENDERER_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_
