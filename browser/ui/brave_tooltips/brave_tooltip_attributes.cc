/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_tooltips/brave_tooltip_attributes.h"

namespace brave_tooltips {

BraveTooltipAttributes::BraveTooltipAttributes(
    const std::u16string& title,
    const std::u16string& body,
    const std::u16string& ok_button_text,
    const std::u16string& cancel_button_text)
    : title_(title),
      body_(body),
      ok_button_text_(ok_button_text),
      cancel_button_text_(cancel_button_text) {
  if (!cancel_button_text.empty()) {
    cancel_button_enabled_ = true;
  }
}

BraveTooltipAttributes::~BraveTooltipAttributes() = default;

BraveTooltipAttributes::BraveTooltipAttributes(
    const BraveTooltipAttributes& other) = default;

BraveTooltipAttributes& BraveTooltipAttributes::operator=(
    const BraveTooltipAttributes& other) = default;

}  // namespace brave_tooltips
