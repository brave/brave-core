/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_DELEGATE_H_
#define BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_DELEGATE_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"

namespace brave_tooltips {

class BraveTooltipDelegate
    : public base::SupportsWeakPtr<BraveTooltipDelegate> {
 public:
  virtual ~BraveTooltipDelegate() = default;

  // Called when the tooltip is shown
  virtual void OnTooltipShow(const std::string& tooltip_id) {}

  // Called when the tooltip is closed. If closed by a user explicitly
  // then |by_user| should be true, otherwise false
  virtual void OnTooltipClose(const std::string& tooltip_id,
                              const bool by_user) {}

  // Called when the underlying widget for the tooltip is destroyed
  virtual void OnTooltipWidgetDestroyed(const std::string& tooltip_id) {}

  // Called when the Ok button is pressed
  virtual void OnTooltipOkButtonPressed(const std::string& tooltip_id) {}

  // Called when the Cancel button is pressed
  virtual void OnTooltipCancelButtonPressed(const std::string& tooltip_id) {}
};

}  // namespace brave_tooltips

#endif  // BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_DELEGATE_H_
