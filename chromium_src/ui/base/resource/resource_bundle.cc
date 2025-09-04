// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/base/resource/resource_bundle.h"

#include "base/containers/flat_set.h"
#include "base/no_destructor.h"

#define GetThemedLottieImageNamed GetThemedLottieImageNamed_ChromiumImpl
#include <ui/base/resource/resource_bundle.cc>
#undef GetThemedLottieImageNamed

namespace ui {

// Runtime set of blocked resource IDs
static base::NoDestructor<base::flat_set<int>> g_blocked_lottie_ids;

COMPONENT_EXPORT(UI_BASE)
void SetBlockedThemedLottieImages(const base::flat_set<int>& blocked_ids) {
  *g_blocked_lottie_ids = blocked_ids;
}

const ui::ImageModel& ResourceBundle::GetThemedLottieImageNamed(
    int resource_id) {
  // Don't display themed assets that are on our block list
  if (g_blocked_lottie_ids->count(resource_id)) {
    static base::NoDestructor<ui::ImageModel> empty_image;
    return *empty_image;
  }

  return GetThemedLottieImageNamed_ChromiumImpl(resource_id);
}

}  // namespace ui
