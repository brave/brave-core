/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a_utils/custom_attributes.h"

#include "brave/components/p3a_utils/event_relay.h"

namespace p3a_utils {

void SetCustomAttribute(std::string_view attribute_name,
                        std::optional<std::string_view> attribute_value) {
  EventRelay::GetInstance()->NotifyCustomAttributeSet(attribute_name,
                                                      attribute_value);
}

}  // namespace p3a_utils
