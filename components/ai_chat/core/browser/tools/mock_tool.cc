// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/mock_tool.h"

#include <utility>

namespace ai_chat {

MockTool::MockTool(std::string_view name,
                   std::string_view description,
                   std::string_view type,
                   std::optional<base::Value::Dict> input_properties,
                   std::optional<std::vector<std::string>> required_properties,
                   std::optional<base::Value::Dict> extra_params)
    : name_(name),
      description_(description),
      type_(type),
      input_properties_(std::move(input_properties)),
      required_properties_(std::move(required_properties)),
      extra_params_(std::move(extra_params)) {}

MockTool::~MockTool() = default;

std::string_view MockTool::Name() const {
  return name_;
}

std::string_view MockTool::Description() const {
  return description_;
}

std::string_view MockTool::Type() const {
  return type_;
}

std::optional<base::Value::Dict> MockTool::InputProperties() const {
  if (input_properties_.has_value()) {
    return input_properties_->Clone();
  }
  return std::nullopt;
}

std::optional<std::vector<std::string>> MockTool::RequiredProperties() const {
  return required_properties_;
}

std::optional<base::Value::Dict> MockTool::ExtraParams() const {
  if (extra_params_.has_value()) {
    return extra_params_->Clone();
  }
  return std::nullopt;
}

}  // namespace ai_chat
