/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ICON_WITH_BADGE_IMAGE_SOURCE_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ICON_WITH_BADGE_IMAGE_SOURCE_H_

#include "chrome/browser/ui/extensions/icon_with_badge_image_source.h"

namespace gfx {
class Canvas;
class Rect;
}  // namespace gfx

namespace brave {

// The purpose of this subclass is to:
// - Paint the BraveAction badge in a custom location and with a different size
//   to regular BrowserAction extensions.
class BraveIconWithBadgeImageSource : public IconWithBadgeImageSource {
 public:
  BraveIconWithBadgeImageSource(
      const gfx::Size& size,
      GetColorProviderCallback get_color_provider_callback,
      size_t content_image_size,
      size_t content_horizontal_margin);

  BraveIconWithBadgeImageSource(const BraveIconWithBadgeImageSource&) = delete;
  BraveIconWithBadgeImageSource& operator=(
      const BraveIconWithBadgeImageSource&) = delete;

  static gfx::Size GetBadgeSize();

 private:
  void PaintBadge(gfx::Canvas* canvas) override;
  gfx::Rect GetIconAreaRect() const override;
  absl::optional<int> GetCustomGraphicSize() override;
  absl::optional<int> GetCustomGraphicXOffset() override;
  absl::optional<int> GetCustomGraphicYOffset() override;

  size_t content_image_size_;
  size_t content_horizontal_margin_;
};

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ICON_WITH_BADGE_IMAGE_SOURCE_H_
