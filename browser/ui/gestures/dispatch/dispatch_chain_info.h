// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_DISPATCH_DISPATCH_CHAIN_INFO_H_
#define BRAVE_BROWSER_UI_GESTURES_DISPATCH_DISPATCH_CHAIN_INFO_H_

#include <optional>

#include "base/functional/callback.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_content_info.h"

namespace gestures {

// One per in-flight `GestureActionDispatcher::DispatchActionsAsync` call, so
// an independently-fired gesture can't clobber another chain still pending.
struct DispatchChainInfo final {
  std::optional<SmartGestureContentInfo> smart_gesture_content;
  base::OnceClosure all_actions_done_callback;
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_DISPATCH_DISPATCH_CHAIN_INFO_H_
