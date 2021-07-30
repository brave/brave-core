/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/content_settings/core/common/content_settings_mojom_traits.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings.mojom.h"

#define RendererContentSettingRules RendererContentSettingRules_ChromiumImpl

#include "../../../../../../components/content_settings/core/common/content_settings_mojom_traits.cc"

#undef RendererContentSettingRules

namespace mojo {

bool StructTraits<content_settings::mojom::RendererContentSettingRulesDataView,
                  RendererContentSettingRules>::
    Read(content_settings::mojom::RendererContentSettingRulesDataView data,
         RendererContentSettingRules* out) {
  return StructTraits<
             content_settings::mojom::RendererContentSettingRulesDataView,
             RendererContentSettingRules_ChromiumImpl>::Read(data, out) &&
         data.ReadAutoplayRules(&out->autoplay_rules) &&
         data.ReadFingerprintingRules(&out->fingerprinting_rules) &&
         data.ReadBraveShieldsRules(&out->brave_shields_rules);
}

}  // namespace mojo
