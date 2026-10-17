// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_TRAFFIC_CONTROL_TRAFFIC_CONTROL_TYPES_H_
#define BRAVE_BROWSER_TRAFFIC_CONTROL_TRAFFIC_CONTROL_TYPES_H_

#include <optional>

#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {
class NavigationHandle;
}  // namespace content

namespace traffic_control {

// Captures navigation state while its NavigationHandle is live so rerouting can
// safely run in a later task.
struct NavigationIntent {
  NavigationIntent(content::NavigationHandle& handle, mojom::TargetPtr target);
  NavigationIntent(NavigationIntent&&) noexcept;
  NavigationIntent& operator=(NavigationIntent&&) noexcept;
  ~NavigationIntent();

  NavigationIntent(const NavigationIntent&) = delete;
  NavigationIntent& operator=(const NavigationIntent&) = delete;

  GURL url;
  mojom::TargetPtr target;
  std::optional<url::Origin> initiator_origin;
  ui::PageTransition page_transition;
  bool user_gesture;
};

}  // namespace traffic_control

#endif  // BRAVE_BROWSER_TRAFFIC_CONTROL_TRAFFIC_CONTROL_TYPES_H_
