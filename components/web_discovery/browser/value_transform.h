/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_VALUE_TRANSFORM_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_VALUE_TRANSFORM_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "base/values.h"

namespace web_discovery {

// Abstract base class for value transformation functions
class ValueTransform {
 public:
  virtual ~ValueTransform() = default;

  // Process the input value
  // Returns nullopt if transformation fails or should stop processing
  virtual std::optional<std::string> Process(std::string_view input) = 0;
};

// Factory function to create transform instances from transform definition
// transform_definition[0] should be the transform name string
std::unique_ptr<ValueTransform> CreateValueTransform(
    const base::Value::List& transform_definition);

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_VALUE_TRANSFORM_H_
