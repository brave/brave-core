// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_HOVERCARD_HOVER_CARD_ANCHOR_TARGET_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_HOVERCARD_HOVER_CARD_ANCHOR_TARGET_H_

#include "brave/components/containers/buildflags/buildflags.h"
#include "ui/base/models/image_model.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
struct ContainerCardData {
  std::u16string container_name;
  ui::ImageModel container_icon;
  SkColor container_background_color;
};
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

#include <chrome/browser/ui/views/tabs/hovercard/hover_card_anchor_target.h>  // IWYU pragma: export

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_HOVERCARD_HOVER_CARD_ANCHOR_TARGET_H_
