// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/traffic_control/traffic_control_types.h"

#include <utility>

#include "content/public/browser/navigation_handle.h"

namespace traffic_control {

NavigationIntent::NavigationIntent(content::NavigationHandle& handle,
                                   mojom::TargetPtr target)
    : url(handle.GetURL()),
      target(std::move(target)),
      initiator_origin(handle.GetInitiatorOrigin()),
      page_transition(handle.GetPageTransition()),
      user_gesture(handle.HasUserGesture()) {}

NavigationIntent::NavigationIntent(NavigationIntent&&) noexcept = default;
NavigationIntent& NavigationIntent::operator=(NavigationIntent&&) noexcept =
    default;
NavigationIntent::~NavigationIntent() = default;

}  // namespace traffic_control
