// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/common/extensions/extension_constants.h"
#include "brave/browser/ui/brave_actions/constants.h"

// Avoid a jump from small image to large image when the non-default
// image is set.
#define BRAVE_GET_EXTENSION_ACTION \
  if (action->default_icon() && ( \
      extension.id() == brave_rewards_extension_id || \
      extension.id() == brave_extension_id)) { \
    action->SetDefaultIconImage(std::make_unique<IconImage>( \
        profile_, &extension, *action->default_icon(), \
        brave_actions::kBraveActionGraphicSize, \
        ExtensionAction::FallbackIcon().AsImageSkia(), nullptr)); \
  }
#include "../../../../../chrome/browser/extensions/extension_action_manager.cc"
#undef BRAVE_GET_EXTENSION_ACTION
