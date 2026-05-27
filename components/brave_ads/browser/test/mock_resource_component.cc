/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/test/mock_resource_component.h"

namespace brave_ads::test {

MockResourceComponent::MockResourceComponent()
    : ResourceComponent(&fake_component_updater_delegate_) {}

MockResourceComponent::~MockResourceComponent() = default;

}  // namespace brave_ads::test
