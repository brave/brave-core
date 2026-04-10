/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a_utils/event_relay.h"

namespace p3a_utils {

// static
EventRelay* EventRelay::GetInstance() {
  static base::NoDestructor<EventRelay> instance;
  return instance.get();
}

EventRelay::EventRelay() = default;
EventRelay::~EventRelay() = default;

void EventRelay::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void EventRelay::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void EventRelay::NotifyCustomAttributeSet(
    std::string_view attribute_name,
    std::optional<std::string_view> attribute_value) {
  observers_.Notify(&Observer::OnCustomAttributeSet, attribute_name,
                    attribute_value);
}

}  // namespace p3a_utils
