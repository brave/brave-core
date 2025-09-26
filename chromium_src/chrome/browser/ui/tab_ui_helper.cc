// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/tab_ui_helper.h"

#include <string>

#include "brave/browser/ui/tabs/features.h"
#include "content/public/browser/navigation_controller.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/canvas.h"
#include "third_party/skia/include/core/SkColor.h"

#define GetTitle GetTitle_ChromiumImpl
#define GetFavicon GetFavicon_ChromiumImpl

#include <chrome/browser/ui/tab_ui_helper.cc>

#undef GetTitle
#undef GetFavicon

void TabUIHelper::SetCustomTitle(const std::optional<std::u16string>& title) {
  if (title == custom_title_) {
    return;
  }

  CHECK(!title.has_value() || !title->empty())
      << "Custom title should be std::nullopt or non-empty string";
  custom_title_ = title;
}

std::u16string TabUIHelper::GetTitle() const {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveRenamingTabs)) {
    return GetTitle_ChromiumImpl();
  }

  return custom_title_.value_or(GetTitle_ChromiumImpl());
}

void TabUIHelper::UpdateLastOrigin() {
  // If the origin has changed since the last time we got the title, reset the
  // custom title. This is to ensure that the custom title is not stale.
  const auto origin =
      web_contents()->GetPrimaryMainFrame()->GetLastCommittedOrigin();
  if (last_origin_ && last_origin_->IsSameOriginWith(origin)) {
    return;
  }

  // In case this tab is newly created one and has not yet commited real page,
  // e.g. restoring tabs, we do not reset the custom title. In case of
  // restoring, the real page will be commited soon and the custom title will be
  // reset or persisted based on the origin of the real page.
  if (web_contents()->GetController().IsInitialNavigation()) {
    return;
  }

  // We reset the custom title only when the last origin is initialized. When
  // restoring tabs, last origin could be uninitialized yet, and we do not want
  // to reset the custom title in that case.
  if (last_origin_) {
    custom_title_.reset();
  }
  last_origin_ = origin;
}

void TabUIHelper::SetCustomEmojiFavicon(
    const std::optional<std::u16string>& emoji) {
  if (emoji == custom_emoji_favicon_) {
    return;
  }
  // Allow empty to clear; if provided, must be non-empty.
  CHECK(!emoji.has_value() || !emoji->empty());
  custom_emoji_favicon_ = emoji;
}

std::optional<std::u16string> TabUIHelper::GetCustomEmojiFaviconString() const {
  return custom_emoji_favicon_;
}

ui::ImageModel TabUIHelper::GetEmojiFaviconImage() const {
  // Render the emoji string to an ImageSkia at favicon size.
  if (!custom_emoji_favicon_ || custom_emoji_favicon_->empty()) {
    return GetFavicon_ChromiumImpl();
  }

  constexpr int kFaviconSize = 16;  // in DIP
  gfx::Canvas canvas(gfx::Size(kFaviconSize, kFaviconSize), /*image_scale=*/1.0f,
                     /*is_opaque=*/false);
  gfx::Rect bounds(0, 0, kFaviconSize, kFaviconSize);

  // Center baseline roughly; emoji often sit slightly below baseline.
  gfx::FontList font_list;
  canvas.DrawStringRectWithFlags(*custom_emoji_favicon_, font_list,
                                 SK_ColorBLACK, bounds,
                                 gfx::Canvas::TEXT_ALIGN_CENTER);

  SkBitmap bitmap = canvas.GetBitmap();
  gfx::ImageSkia image_skia = gfx::ImageSkia::CreateFrom1xBitmap(bitmap);
  return ui::ImageModel::FromImageSkia(image_skia);
}

ui::ImageModel TabUIHelper::GetFavicon() const {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveEmojiTabFavicon)) {
    return GetFavicon_ChromiumImpl();
  }
  if (custom_emoji_favicon_) {
    return GetEmojiFaviconImage();
  }
  return GetFavicon_ChromiumImpl();
}
