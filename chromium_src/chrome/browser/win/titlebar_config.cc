/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/win/titlebar_config.cc"

#include "base/feature_override.h"

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kWindows11MicaTitlebar, base::FEATURE_DISABLED_BY_DEFAULT},
}});
