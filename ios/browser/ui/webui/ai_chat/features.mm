// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/ai_chat/features.h"

#include "base/feature_list.h"

namespace ai_chat::features {

BASE_FEATURE(kAIChatWebUIEnabled,
             base::FEATURE_DISABLED_BY_DEFAULT);

bool IsAIChatWebUIEnabled() {
  return base::FeatureList::IsEnabled(kAIChatWebUIEnabled);
}

}  // namespace ai_chat::features
