/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_DELEGATE_H_
#define BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_DELEGATE_H_

#include "base/memory/ref_counted.h"

namespace brave_tooltips {

class TooltipObserver {
 public:
  // Called when the tooltip is shown
  virtual void OnShow() {}

  // Called when the tooltip is closed. If closed by a user explicitly
  // then |by_user| should be true, otherwise false
  virtual void OnClose(const bool by_user) {}

  // Called when the Ok button is pressed
  virtual void OnOkButtonPressed() {}

  // Called when the Cancel button is pressed
  virtual void OnCancelButtonPressed() {}
};

// Ref counted version of TooltipObserver, required to satisfy
// brave_tooltips::BraveTooltip::delegate_
class BraveTooltipDelegate
    : public TooltipObserver,
      public base::RefCountedThreadSafe<BraveTooltipDelegate> {
 protected:
  virtual ~BraveTooltipDelegate() = default;

 private:
  friend class base::RefCountedThreadSafe<BraveTooltipDelegate>;
};

}  // namespace brave_tooltips

#endif  // BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_DELEGATE_H_
