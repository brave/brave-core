// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/permissions/contexts/brave_ai_chat_permission_context.h"

#include "components/content_settings/core/common/content_settings_types.h"
#include "third_party/blink/public/mojom/permissions_policy/permissions_policy.mojom.h"

namespace permissions {

BraveAIChatPermissionContext::BraveAIChatPermissionContext(
    content::BrowserContext* browser_context)
    : PermissionContextBase(browser_context,
                            ContentSettingsType::BRAVE_AI_CHAT,
                            blink::mojom::PermissionsPolicyFeature::kNotFound) {
}

BraveAIChatPermissionContext::~BraveAIChatPermissionContext() = default;

bool BraveAIChatPermissionContext::IsRestrictedToSecureOrigins() const {
  return true;
}

}  // namespace permissions
