/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_UTILS_EVENT_RELAY_H_
#define BRAVE_COMPONENTS_P3A_UTILS_EVENT_RELAY_H_

#include <optional>
#include <string>
#include <string_view>

#include "base/no_destructor.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/scoped_observation.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"

namespace p3a_utils {

class EventRelay {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnCustomAttributeSet(
        std::string_view attribute_name,
        std::optional<std::string_view> attribute_value) = 0;
  };

  static EventRelay* GetInstance();

  EventRelay(const EventRelay&) = delete;
  EventRelay& operator=(const EventRelay&) = delete;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void NotifyCustomAttributeSet(
      std::string_view attribute_name,
      std::optional<std::string_view> attribute_value);

 private:
  friend class base::NoDestructor<EventRelay>;

  EventRelay();
  ~EventRelay();

  base::ObserverList<Observer> observers_;
};

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

#endif  // BRAVE_COMPONENTS_P3A_UTILS_EVENT_RELAY_H_
