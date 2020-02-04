/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/permissions/brave_permission_manager.h"

#include <memory>

#include "brave/browser/autoplay/autoplay_permission_context.h"
#include "components/content_settings/core/common/content_settings_types.h"

BravePermissionManager::BravePermissionManager(Profile* profile)
  : PermissionManager(profile) {
  permission_contexts_[ContentSettingsType::AUTOPLAY] =
      std::make_unique<AutoplayPermissionContext>(profile);
}
