/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/content_settings/renderer/content_settings_agent_impl.h"

#define BRAVE_CONTENT_SETTINGS_AGENT_IMPL_IS_WHITELISTED_FOR_CONTENT_SETTINGS \
  return IsAllowlistedForContentSettings(origin, document_url);               \
}                                                                             \
                                                                              \
bool ContentSettingsAgentImpl::IsAllowlistedForContentSettings(               \
     const WebSecurityOrigin& origin, const WebURL& document_url) const {
// #define BRAVE_CONTENT_SETTINGS_AGENT_IMPL_IS_WHITELISTED_FOR_CONTENT_SETTINGS

#include "../../../../../components/content_settings/renderer/content_settings_agent_impl.cc"
#undef IsAllowlistedForContentSettings
