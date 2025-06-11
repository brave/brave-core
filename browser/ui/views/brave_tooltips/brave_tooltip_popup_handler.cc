/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_tooltips/brave_tooltip_popup_handler.h"

#include <map>
#include <string>

#include "base/check.h"
#include "brave/browser/ui/brave_tooltips/brave_tooltip.h"
#include "brave/browser/ui/views/brave_tooltips/brave_tooltip_popup.h"

namespace {

std::map<std::string, brave_tooltips::BraveTooltipPopup* /* NOT OWNED */>
    tooltip_popups_;

}  // namespace

namespace brave_tooltips {

BraveTooltipPopupHandler::BraveTooltipPopupHandler() = default;

BraveTooltipPopupHandler::~BraveTooltipPopupHandler() = default;

// static
void BraveTooltipPopupHandler::Show(std::unique_ptr<BraveTooltip> tooltip) {
  DCHECK(tooltip);

  const std::string tooltip_id = tooltip->id();
  if (!tooltip_popups_[tooltip_id]) {
    tooltip_popups_[tooltip_id] = new BraveTooltipPopup(std::move(tooltip));
    tooltip_popups_[tooltip_id]->Show();
  }
}

// static
void BraveTooltipPopupHandler::Close(const std::string& tooltip_id) {
  DCHECK(!tooltip_id.empty());

  if (!tooltip_popups_[tooltip_id]) {
    return;
  }

  tooltip_popups_[tooltip_id]->Close();
}

// static
void BraveTooltipPopupHandler::Destroy(const std::string& tooltip_id) {
  DCHECK(!tooltip_id.empty());

  // Note: The pointed-to BraveTooltipPopup members are deallocated by their
  // containing Widgets
  tooltip_popups_.erase(tooltip_id);
}

}  // namespace brave_tooltips
