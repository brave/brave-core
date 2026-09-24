/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/profile.h"

#define BRAVE_BUILD_HISTORY_SERVICE          \
  history_service->InitHistoryRetentionPref( \
      Profile::FromBrowserContext(context)->GetPrefs());

#include <chrome/browser/history/history_service_factory.cc>

#undef BRAVE_BUILD_HISTORY_SERVICE
