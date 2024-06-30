/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/content_settings/generated_notification_pref.h"

#define kTpcdGrant \
  kRemoteList:     \
  case SettingSource::kTpcdGrant

#include "src/chrome/browser/content_settings/generated_notification_pref_unittest.cc"

#undef kTpcdGrant
