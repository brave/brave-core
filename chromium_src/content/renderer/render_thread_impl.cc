/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define REGISTER_BRAVE_SCHEMES_DISPLAY_ISOLATED_AND_NO_JS                      \
  WebString brave_scheme(WebString::FromASCII(kBraveUIScheme));                \
  WebSecurityPolicy::RegisterURLSchemeAsDisplayIsolated(brave_scheme);         \
  WebSecurityPolicy::RegisterURLSchemeAsNotAllowingJavascriptURLs(             \
      brave_scheme);                                                           \
                                                                               \
  WebString binance_scheme(WebString::FromASCII(kBinanceScheme));              \
  WebSecurityPolicy::RegisterURLSchemeAsDisplayIsolated(binance_scheme);       \
  WebSecurityPolicy::RegisterURLSchemeAsNotAllowingJavascriptURLs(             \
      binance_scheme);                                                         \
                                                                               \
  WebString gemini_scheme(WebString::FromASCII(kGeminiScheme));                \
  WebSecurityPolicy::RegisterURLSchemeAsDisplayIsolated(gemini_scheme);        \
  WebSecurityPolicy::RegisterURLSchemeAsNotAllowingJavascriptURLs(             \
      gemini_scheme);                                                          \
                                                                               \
  WebString ftx_scheme(WebString::FromASCII(kFTXScheme));                      \
  WebSecurityPolicy::RegisterURLSchemeAsDisplayIsolated(ftx_scheme);           \
  WebSecurityPolicy::RegisterURLSchemeAsNotAllowingJavascriptURLs(ftx_scheme); \
                                                                               \
  WebString ipfs_scheme(WebString::FromASCII("ipfs"));                         \
  WebSecurityPolicy::RegisterURLSchemeAsSupportingFetchAPI(ipfs_scheme);       \
                                                                               \
  WebString ipns_scheme(WebString::FromASCII("ipns"));                         \
  WebSecurityPolicy::RegisterURLSchemeAsSupportingFetchAPI(ipns_scheme);

#include "src/content/renderer/render_thread_impl.cc"
