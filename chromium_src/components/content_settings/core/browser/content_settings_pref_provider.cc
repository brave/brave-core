/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"

#define BRAVE_SET_WEBSITE_SETTINGS                                     \
      !content_settings::IsShieldsContentSettingsType(content_type) &&

#include "../../../../../../components/content_settings/core/browser/content_settings_pref_provider.cc"

#undef BRAVE_SET_WEBSITE_SETTINGS
