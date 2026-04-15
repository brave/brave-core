/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a_utils/test_event_relay_observer.h"

#include "base/containers/map_util.h"

namespace p3a_utils {

TestEventRelayObserver::TestEventRelayObserver() {
  observation_.Observe(EventRelay::GetInstance());
}

TestEventRelayObserver::~TestEventRelayObserver() = default;

std::optional<std::string> TestEventRelayObserver::GetCustomAttribute(
    std::string_view attribute_name) const {
  const auto* value = base::FindOrNull(attributes_, attribute_name);
  return value ? *value : std::nullopt;
}

void TestEventRelayObserver::OnCustomAttributeSet(
    std::string_view attribute_name,
    std::optional<std::string_view> attribute_value) {
  attributes_[std::string(attribute_name)] =
      attribute_value ? std::optional<std::string>(*attribute_value)
                      : std::nullopt;
}

}  // namespace p3a_utils
