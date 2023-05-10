/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ssl/https_only_mode_upgrade_interceptor.h"

#include "net/base/url_util.h"

#define IsLocalhost(URL) IsLocalhostOrOnion(URL)

#include "src/chrome/browser/ssl/https_only_mode_upgrade_interceptor.cc"

#undef IsLocalhost
