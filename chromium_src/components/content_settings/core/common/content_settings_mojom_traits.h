/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_MOJOM_TRAITS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_MOJOM_TRAITS_H_

#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings.mojom.h"

#define RendererContentSettingRules RendererContentSettingRules_ChromiumImpl

#include "../../../../../../components/content_settings/core/common/content_settings_mojom_traits.h"

#undef RendererContentSettingRules

namespace mojo {

template <>
struct StructTraits<
    content_settings::mojom::RendererContentSettingRulesDataView,
    RendererContentSettingRules>
    : public StructTraits<
          content_settings::mojom::RendererContentSettingRulesDataView,
          RendererContentSettingRules_ChromiumImpl> {
  static const std::vector<ContentSettingPatternSource>& autoplay_rules(
      const RendererContentSettingRules& r) {
    return r.autoplay_rules;
  }
  static const std::vector<ContentSettingPatternSource>& fingerprinting_rules(
      const RendererContentSettingRules& r) {
    return r.fingerprinting_rules;
  }
  static const std::vector<ContentSettingPatternSource>& brave_shields_rules(
      const RendererContentSettingRules& r) {
    return r.brave_shields_rules;
  }

  static bool Read(
      content_settings::mojom::RendererContentSettingRulesDataView data,
      RendererContentSettingRules* out);
};

}  // namespace mojo

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_MOJOM_TRAITS_H_
