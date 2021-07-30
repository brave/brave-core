/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/safe_browsing/core/common/safe_browsing_prefs.h"

#define BRAVE_IS_ENHANCED_PROTECTION_ENABLED return false;
#include "../../../../../../components/safe_browsing/core/common/safe_browsing_prefs.cc"
#undef BRAVE_IS_ENHANCED_PROTECTION_ENABLED
