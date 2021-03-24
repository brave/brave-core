/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/signin/profile_picker_ui.h"

#include "brave/grit/brave_generated_resources.h"
#include "chrome/grit/generated_resources.h"

#undef IDS_DEFAULT_AVATAR_LABEL_26
#define IDS_DEFAULT_AVATAR_LABEL_26 IDS_BRAVE_AVATAR_LABEL_PLACEHOLDER
#include "../../../../../../../chrome/browser/ui/webui/signin/profile_picker_ui.cc"
#undef IDS_DEFAULT_AVATAR_LABEL_26
