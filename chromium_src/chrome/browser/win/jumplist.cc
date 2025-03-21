/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/grit/brave_generated_resources.h"
#include "chrome/grit/generated_resources.h"

#define IDS_NEW_INCOGNITO_WINDOW_OLD IDS_NEW_INCOGNITO_WINDOW
#undef IDS_NEW_INCOGNITO_WINDOW
#define IDS_NEW_INCOGNITO_WINDOW IDS_BRAVE_NEW_INCOGNITO_WINDOW
#define IDS_NEW_OFFTHERECORD_WINDOW_TOR IDS_BRAVE_NEW_OFFTHERECORD_WINDOW_TOR
#include "src/chrome/browser/win/jumplist.cc"
#undef IDS_NEW_OFFTHERECORD_WINDOW_TOR
#undef IDS_NEW_INCOGNITO_WINDOW
#define IDS_NEW_INCOGNITO_WINDOW IDS_NEW_INCOGNITO_WINDOW_OLD
#undef IDS_NEW_INCOGNITO_WINDOW_OLD
