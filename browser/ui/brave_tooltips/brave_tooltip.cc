/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_tooltips/brave_tooltip.h"

#include <vector>

#include "base/strings/string_util.h"

namespace brave_tooltips {

BraveTooltip::BraveTooltip(const std::string& id,
                           const BraveTooltipAttributes& attributes,
                           base::WeakPtr<BraveTooltipDelegate> delegate)
    : id_(id), attributes_(attributes), delegate_(std::move(delegate)) {}

BraveTooltip::~BraveTooltip() = default;

std::u16string BraveTooltip::accessible_name() const {
  std::vector<std::u16string> accessible_lines;

  if (!attributes_.title().empty()) {
    accessible_lines.push_back(attributes_.title());
  }

  if (!attributes_.body().empty()) {
    accessible_lines.push_back(attributes_.body());
  }

  return base::JoinString(accessible_lines, u"\n");
}

}  // namespace brave_tooltips
