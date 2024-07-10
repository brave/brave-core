/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define REGISTER_BRAVE_SCHEMES_DISPLAY_ISOLATED_AND_NO_JS                \
  WebString brave_scheme(WebString::FromASCII(kBraveUIScheme));          \
  WebSecurityPolicy::RegisterURLSchemeAsDisplayIsolated(brave_scheme);   \
  WebSecurityPolicy::RegisterURLSchemeAsNotAllowingJavascriptURLs(       \
      brave_scheme);                                                     \

#include "src/content/renderer/render_thread_impl.cc"
#undef REGISTER_BRAVE_SCHEMES_DISPLAY_ISOLATED_AND_NO_JS
