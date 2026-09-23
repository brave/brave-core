/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_POPUP_HANDLER_H_
#define BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_POPUP_HANDLER_H_

#include <memory>
#include <string>

namespace brave_tooltips {

class BraveTooltip;

class BraveTooltipPopupHandler {
 public:
  BraveTooltipPopupHandler();
  ~BraveTooltipPopupHandler();

  BraveTooltipPopupHandler(const BraveTooltipPopupHandler&) = delete;
  BraveTooltipPopupHandler& operator=(const BraveTooltipPopupHandler&) = delete;

  // Show the |tooltip|.
  static void Show(std::unique_ptr<BraveTooltip> tooltip);

  // Close the tooltip with the associated |tooltip_id|.
  static void Close(const std::string& tooltip_id);

  // Destroy the tooltip with the associated |tooltip_id|.
  static void Destroy(const std::string& tooltip_id);
};

}  // namespace brave_tooltips

#endif  // BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_POPUP_HANDLER_H_
