/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "build/branding_buildflags.h"

#include "brave/services/network/public/cpp/system_request_handler.h"

// This is currently necessary in order to enable Certificate Transparency
// enforcement (brave-browser#22482).
#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#define BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING() (1)
#undef OFFICIAL_BUILD
#define OFFICIAL_BUILD
#include "src/chrome/browser/net/system_network_context_manager.cc"
