/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_BROWSER_H \
 private: \
  friend class BraveHostContentSettingsMap;

#include "../../../../../../components/content_settings/core/browser/host_content_settings_map.h"

#undef BRAVE_BROWSER_H
