/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/content_settings/browser/content_settings_manager_impl.h"
#include "components/content_settings/core/common/cookie_settings_base.h"

#define IsCookieAccessAllowed IsChromiumCookieAccessAllowed
#include "../../../../../components/content_settings/browser/content_settings_manager_impl.cc"
#undef IsCookieAccessAllowed
