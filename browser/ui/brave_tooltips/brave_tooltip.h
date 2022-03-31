/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_H_
#define BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_H_

#include <string>
#include <utility>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/brave_tooltips/brave_tooltip_attributes.h"
#include "brave/browser/ui/brave_tooltips/brave_tooltip_delegate.h"

namespace brave_tooltips {

class BraveTooltip {
 public:
  // Create a new tooltip with an |id| and |attributes|.  |delegate|
  // will influence the behaviour of this tooltip and receives events
  // on its behalf. The delegate may be omitted
  BraveTooltip(const std::string& id,
               const BraveTooltipAttributes& attributes,
               base::WeakPtr<BraveTooltipDelegate> delegate);
  virtual ~BraveTooltip();

  BraveTooltip(const BraveTooltip&) = delete;
  BraveTooltip& operator=(const BraveTooltip&) = delete;

  const std::string& id() const { return id_; }

  const BraveTooltipAttributes& attributes() const { return attributes_; }
  void set_attributes(const BraveTooltipAttributes& attributes) {
    attributes_ = attributes;
  }

  std::u16string accessible_name() const;

  BraveTooltipDelegate* delegate() const { return delegate_.get(); }

  void set_delegate(base::WeakPtr<BraveTooltipDelegate> delegate) {
    DCHECK(!delegate_);
    delegate_ = std::move(delegate);
  }

  virtual void PerformOkButtonAction() {}
  virtual void PerformCancelButtonAction() {}

 protected:
  std::string id_;
  BraveTooltipAttributes attributes_;

 private:
  base::WeakPtr<BraveTooltipDelegate> delegate_;
};

}  // namespace brave_tooltips

#endif  // BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_H_
