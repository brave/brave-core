// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/tab_ui_helper.h"

#include <string>

#include "brave/browser/ui/tabs/features.h"
#include "content/public/browser/navigation_controller.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"
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
  const auto current_origin =
      web_contents()->GetPrimaryMainFrame()->GetLastCommittedOrigin();
  const GURL current_url = web_contents()->GetLastCommittedURL();

  // Skip initial navigations (e.g., restoring tabs) to avoid premature resets.
  if (web_contents()->GetController().IsInitialNavigation()) {
    return;
  }

  if (last_origin_) {
    const GURL last_url = last_origin_->GetURL();
    using net::registry_controlled_domains::GetDomainAndRegistry;
    using net::registry_controlled_domains::PrivateRegistryFilter;
    constexpr PrivateRegistryFilter kFilter =
        net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES;

    std::string last_base = GetDomainAndRegistry(last_url, kFilter);
    if (last_base.empty()) {
      last_base = last_url.host();
    }

    std::string current_base = GetDomainAndRegistry(current_url, kFilter);
    if (current_base.empty()) {
      current_base = current_url.host();
    }

    // First, if we stored a base domain at the time of setting the emoji,
    // require a change relative to that specific base to reset. This avoids
    // accidental resets on redirects within the same site group.
    if (custom_emoji_favicon_base_domain_ &&
        *custom_emoji_favicon_base_domain_ != current_base) {
      custom_emoji_favicon_.reset();
      custom_emoji_favicon_base_domain_.reset();
    }

    // Also reset custom title on base-domain change.
    if (last_base != current_base) {
      // Base domain changed: clear custom state that is site-specific.
      custom_title_.reset();
      custom_emoji_favicon_.reset();
    }
  }

  last_origin_ = current_origin;
}

void TabUIHelper::SetCustomEmojiFavicon(
    const std::optional<std::u16string>& emoji) {
  if (emoji == custom_emoji_favicon_) {
    return;
  }
  // Allow empty to clear; if provided, must be non-empty.
  CHECK(!emoji.has_value() || !emoji->empty());
  custom_emoji_favicon_ = emoji;

  // Track the base domain at the time the emoji was set, for reset purposes.
  if (custom_emoji_favicon_) {
    using net::registry_controlled_domains::GetDomainAndRegistry;
    using net::registry_controlled_domains::PrivateRegistryFilter;
    constexpr PrivateRegistryFilter kFilter =
        net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES;
    const GURL url = web_contents()->GetLastCommittedURL();
    std::string base = GetDomainAndRegistry(url, kFilter);
    if (base.empty()) {
      base = url.host();
    }
    custom_emoji_favicon_base_domain_ = base;
  } else {
    custom_emoji_favicon_base_domain_.reset();
  }
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

std::optional<std::string> TabUIHelper::GetCustomEmojiBaseDomainForReset() const {
  return custom_emoji_favicon_base_domain_;
}
