/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_COOKIE_JAR_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_COOKIE_JAR_H_

#define BRAVE_COOKIE_JAR_H                                  \
 public:                                                    \
  bool IsEphemeralCookieAllowed();                          \
 private:                                                   \
  bool ChromiumCookiesEnabled();                            \
  bool ShouldUseEphemeralCookie();

#include "../../../../../../../third_party/blink/renderer/core/loader/cookie_jar.h"

#undef BRAVE_COOKIE_JAR_H

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_COOKIE_JAR_H_
