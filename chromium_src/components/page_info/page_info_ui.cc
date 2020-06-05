/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/vector_icons/vector_icons.h"

#define BRAVE_PAGE_INFO_UI_GET_PERMISSION_ICON \
  case ContentSettingsType::AUTOPLAY:          \
    icon = &kAutoplayStatusIcon;               \
    break;

#include "../../../../components/page_info/page_info_ui.cc"
#undef BRAVE_PAGE_INFO_UI_GET_PERMISSION_ICON
