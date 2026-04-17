/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_UTILS_TEST_EVENT_RELAY_OBSERVER_H_
#define BRAVE_COMPONENTS_P3A_UTILS_TEST_EVENT_RELAY_OBSERVER_H_

#include <optional>
#include <string>
#include <string_view>

#include "base/scoped_observation.h"
#include "brave/components/p3a_utils/event_relay.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"

namespace p3a_utils {

// Test helper that observes EventRelay and caches custom attribute values.
class TestEventRelayObserver : public EventRelay::Observer {
 public:
  TestEventRelayObserver();
  ~TestEventRelayObserver() override;

  // Returns the cached value for |attribute_name|, or std::nullopt if the
  // attribute was cleared or never set.
  std::optional<std::string> GetCustomAttribute(
      std::string_view attribute_name) const;

  // EventRelay::Observer
  void OnCustomAttributeSet(
      std::string_view attribute_name,
      std::optional<std::string_view> attribute_value) override;

 private:
  absl::flat_hash_map<std::string, std::optional<std::string>> attributes_;
  base::ScopedObservation<EventRelay, EventRelay::Observer> observation_{this};
};

}  // namespace p3a_utils

#endif  // BRAVE_COMPONENTS_P3A_UTILS_TEST_EVENT_RELAY_OBSERVER_H_
