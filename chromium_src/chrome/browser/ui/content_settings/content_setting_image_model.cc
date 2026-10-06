/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/content_settings/content_setting_image_model.h"

#include <algorithm>
#include <array>

#include "base/check_op.h"
#include "base/containers/span.h"
#include "brave/browser/ui/content_settings/brave_content_setting_image_models.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/gfx/vector_icon_types.h"

namespace {

// Returns whether `type` uses a Brave icon, setting `icon` and `badge` if so.
bool BraveGetIconFromType(ContentSettingsType type,
                          bool blocked,
                          raw_ptr<const gfx::VectorIcon>* icon,
                          raw_ptr<const gfx::VectorIcon>* badge) {
  if (type != ContentSettingsType::AUTOPLAY) {
    return false;
  }
  *badge = blocked ? &vector_icons::kBlockedBadgeCustomIcon
                   : &gfx::VectorIcon::EmptyIcon();
  *icon = &kAutoplayStatusIcon;
  return true;
}

constexpr bool IsShownByBrave(ContentSettingImageModel::ImageType type) {
  return !std::ranges::contains(kBraveRemovedContentSettingImageTypes, type);
}

// Returns the element identifiers of `kImageOrder`, less the image models
// Brave removes. Brave's autoplay model reuses kMediaStream's image type, so it
// adds no identifier of its own.
template <const auto& kImageOrder, auto kGetElementIdentifier>
base::span<const ui::ElementIdentifier> BraveGetAllElementIdentifiers() {
  static constexpr auto kIdentifiers = []() consteval {
    std::array<ui::ElementIdentifier,
               std::ranges::count_if(kImageOrder, IsShownByBrave)>
        result;
    size_t i = 0;
    for (ContentSettingImageModel::ImageType type : kImageOrder) {
      if (IsShownByBrave(type)) {
        result[i++] = kGetElementIdentifier(type);
      }
    }
    CHECK_EQ(i, result.size());
    return result;
  }();
  return kIdentifiers;
}

}  // namespace

#include <chrome/browser/ui/content_settings/content_setting_image_model.cc>
