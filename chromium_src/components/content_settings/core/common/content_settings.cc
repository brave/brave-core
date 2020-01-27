/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_IS_RENDERER_CONTENT_SETTING \
  content_type == ContentSettingsType::AUTOPLAY ||

#include "../../../../../components/content_settings/core/common/content_settings.cc"  // NOLINT

#undef BRAVE_IS_RENDERER_CONTENT_SETTING
