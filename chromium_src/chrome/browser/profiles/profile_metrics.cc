// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "chrome/browser/profiles/profile_metrics.h"

#define LogProfileAvatarSelection LogProfileAvatarSelection_ChromiumImpl
#include "src/chrome/browser/profiles/profile_metrics.cc"
#undef LogProfileAvatarSelection

// Chromium attempts to log profile icons which do not include
// Brave's additions in profile_avatar_icon_util.cc
// Brave does not need this histogram, so we don't do anything here.
// If we do want this histogram in the future then we can handle if the index
// is greater than chromium's max.
void ProfileMetrics::LogProfileAvatarSelection(size_t icon_index) { }
