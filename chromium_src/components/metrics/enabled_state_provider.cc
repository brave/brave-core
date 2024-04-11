/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/metrics/enabled_state_provider.h"

// Consent is only for chromium metrics services which we disable
#define IsConsentGiven() false
#include "src/components/metrics/enabled_state_provider.cc"
#undef IsConsentGiven
