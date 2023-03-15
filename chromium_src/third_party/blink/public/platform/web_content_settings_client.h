/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_CONTENT_SETTINGS_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_CONTENT_SETTINGS_CLIENT_H_

#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "third_party/blink/public/platform/web_security_origin.h"

class GURL;

#define AllowStorageAccessSync                                         \
  AllowAutoplay(bool play_requested) {                                 \
    return true;                                                       \
  }                                                                    \
  virtual bool IsCosmeticFilteringEnabled(const GURL& url) {           \
    return false;                                                      \
  }                                                                    \
  virtual bool IsFirstPartyCosmeticFilteringEnabled(const GURL& url) { \
    return false;                                                      \
  }                                                                    \
  virtual BraveFarblingLevel GetBraveFarblingLevel() {                 \
    return BraveFarblingLevel::OFF;                                    \
  }                                                                    \
  virtual bool IsReduceLanguageEnabled() {                             \
    return false;                                                      \
  }                                                                    \
  virtual blink::WebSecurityOrigin GetEphemeralStorageOriginSync() {   \
    return blink::WebSecurityOrigin();                                 \
  }                                                                    \
  virtual bool HasContentSettingsRules() const {                       \
    return false;                                                      \
  }                                                                    \
  virtual bool AllowStorageAccessSync

#include "src/third_party/blink/public/platform/web_content_settings_client.h"  // IWYU pragma: export

#undef AllowStorageAccessSync

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_CONTENT_SETTINGS_CLIENT_H_
