/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_RENDERER_CONTENT_SETTINGS_AGENT_IMPL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_RENDERER_CONTENT_SETTINGS_AGENT_IMPL_H_

#include "third_party/blink/public/platform/web_content_settings_client.h"

#define BRAVE_CONTENT_SETTINGS_AGENT_IMPL_H_ \
  friend class BraveContentSettingsAgentImpl;

#define ShouldAutoupgradeMixedContent       \
  HasContentSettingsRules() const override; \
  bool ShouldAutoupgradeMixedContent

#include "src/components/content_settings/renderer/content_settings_agent_impl.h"  // IWYU pragma: export
#undef ShouldAutoupgradeMixedContent
#undef BRAVE_CONTENT_SETTINGS_AGENT_IMPL_H_

namespace content_settings {

ContentSetting GetContentSettingFromRulesImpl(
    const ContentSettingsForOneType& rules,
    const GURL& secondary_url);

}  // namespace content_settings

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_RENDERER_CONTENT_SETTINGS_AGENT_IMPL_H_
