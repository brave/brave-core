// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_UI_BASE_RESOURCE_RESOURCE_BUNDLE_H_
#define BRAVE_CHROMIUM_SRC_UI_BASE_RESOURCE_RESOURCE_BUNDLE_H_

#include "base/component_export.h"
#include "base/containers/flat_set.h"

#define GetThemedLottieImageNamed             \
  GetThemedLottieImageNamed(int resource_id); \
  const ui::ImageModel& GetThemedLottieImageNamed_ChromiumImpl
#include <ui/base/resource/resource_bundle.h>  // IWYU pragma: export
#undef GetThemedLottieImageNamed

namespace ui {

COMPONENT_EXPORT(UI_BASE)
void SetBlockedThemedLottieImages(const base::flat_set<int>& blocked_ids);

}  // namespace ui

#endif  // BRAVE_CHROMIUM_SRC_UI_BASE_RESOURCE_RESOURCE_BUNDLE_H_
