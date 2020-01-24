/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_MOJOM_TRAITS_CC_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_MOJOM_TRAITS_CC_

#define BRAVE_READ_RENDERER_CONTENT_SETTING_RULES_DATA_VIEW       \
  data.ReadAutoplayRules(&out->autoplay_rules) &&                 \
      data.ReadFingerprintingRules(&out->fingerprinting_rules) && \
      data.ReadBraveShieldsRules(&out->brave_shields_rules)&&

#include "../../../../../components/content_settings/core/common/content_settings_mojom_traits.cc"  // NOLINT

#undef BRAVE_READ_RENDERER_CONTENT_SETTING_RULES_DATA_VIEW

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_MOJOM_TRAITS_CC_
