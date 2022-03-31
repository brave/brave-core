/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_ACTIONS_BRAVE_ACTION_ICON_WITH_BADGE_IMAGE_SOURCE_H_
#define BRAVE_BROWSER_UI_BRAVE_ACTIONS_BRAVE_ACTION_ICON_WITH_BADGE_IMAGE_SOURCE_H_

#include "chrome/browser/ui/extensions/icon_with_badge_image_source.h"

namespace gfx {
class Canvas;
class Rect;
}

// The purpose of this subclass is to:
// - Paint the BraveAction badge in a custom location and with a different size
//   to regular BrowserAction extensions.
class BraveActionIconWithBadgeImageSource : public IconWithBadgeImageSource {
 public:
  using IconWithBadgeImageSource::IconWithBadgeImageSource;

  BraveActionIconWithBadgeImageSource(
      const BraveActionIconWithBadgeImageSource&) = delete;
  BraveActionIconWithBadgeImageSource& operator=(
      const BraveActionIconWithBadgeImageSource&) = delete;

 private:
  absl::optional<int> GetCustomGraphicSize() override;
  absl::optional<int> GetCustomGraphicXOffset() override;
  absl::optional<int> GetCustomGraphicYOffset() override;
  void PaintBadge(gfx::Canvas* canvas) override;
  gfx::Rect GetIconAreaRect() const override;
};

#endif  // BRAVE_BROWSER_UI_BRAVE_ACTIONS_BRAVE_ACTION_ICON_WITH_BADGE_IMAGE_SOURCE_H_
