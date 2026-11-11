// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "chrome/browser/profiles/profile_metrics.h"

#define LogProfileAvatarSelection LogProfileAvatarSelection_ChromiumImpl
#define LogProfileAvatarOnLoad LogProfileAvatarOnLoad_ChromiumImpl
#include <chrome/browser/profiles/profile_metrics.cc>
#undef LogProfileAvatarSelection
#undef LogProfileAvatarOnLoad

// Chromium attempts to log profile icons which do not include Brave's additions
// in profile_avatar_icon_util.cc, so the upstream implementations hit
// NOTREACHED() for Brave's avatar indices. Brave does not need these
// histograms, so we don't do anything here. If we do want these histograms in
// the future then we can handle if the index is greater than chromium's max.
void ProfileMetrics::LogProfileAvatarSelection(size_t icon_index) { }
void ProfileMetrics::LogProfileAvatarOnLoad(size_t icon_index) { }
