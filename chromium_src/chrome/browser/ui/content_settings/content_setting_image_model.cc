/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/content_settings/content_setting_image_model.h"

#include <algorithm>
#include <vector>

#include "base/containers/span.h"
#include "base/no_destructor.h"
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

base::span<const ui::ElementIdentifier> BraveGetAllElementIdentifiers() {
  // Derive the identifiers from our model list, as upstream's implementation
  // reports the models we remove. Autoplay shares kMediaStream's identifier,
  // so skip duplicates.
  static const base::NoDestructor<std::vector<ui::ElementIdentifier>>
      kIdentifiers([] {
        std::vector<ui::ElementIdentifier> result;
        for (const auto& model :
             ContentSettingImageModel::GenerateContentSettingImageModels()) {
          const ui::ElementIdentifier identifier =
              model->GetElementIdentifier();
          if (!std::ranges::contains(result, identifier)) {
            result.push_back(identifier);
          }
        }
        return result;
      }());
  return *kIdentifiers;
}

}  // namespace

#include <chrome/browser/ui/content_settings/content_setting_image_model.cc>
