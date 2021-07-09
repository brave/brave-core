/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_COOKIE_SETTINGS_GET_COOKIE_SETTINGS_INTERNAL \
  if (cookie_setting == CONTENT_SETTING_SESSION_ONLY) {    \
    /* Do nothing */                                       \
  } else  // NOLINT

#include "../../../../services/network/cookie_settings.cc"

#undef BRAVE_COOKIE_SETTINGS_GET_COOKIE_SETTINGS_INTERNAL
