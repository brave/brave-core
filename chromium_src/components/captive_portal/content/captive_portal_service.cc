/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/captive_portal/content/captive_portal_service.h"

#define DISABLED_FOR_TESTING \
  DISABLED_FOR_TESTING;      \
  true
#include "src/components/captive_portal/content/captive_portal_service.cc"
#undef DISABLED_FOR_TESTING
