/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/captive_portal/core/captive_portal_detector.h"

#define kDefaultURL                                           \
  kDefaultURL[] = "http://detectportal.brave-http-only.com/"; \
  const char kEmpty
#include "src/components/captive_portal/core/captive_portal_detector.cc"
#undef kDefaultURL
