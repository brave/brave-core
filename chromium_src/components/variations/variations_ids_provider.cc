/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/variations/variations_ids_provider.h"

#define GetClientDataHeaders GetClientDataHeaders_Chromium
#include "src/components/variations/variations_ids_provider.cc"
#undef GetClientDataHeaders
