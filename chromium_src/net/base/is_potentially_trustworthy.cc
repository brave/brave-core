/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/base/is_potentially_trustworthy.h"

#define IsLocalhost(URL) IsLocalhostOrOnion(URL)

#include <net/base/is_potentially_trustworthy.cc>

#undef IsLocalhost
