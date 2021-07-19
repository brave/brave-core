/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_H_
#define BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_H_

#include <string>
#include <utility>

#include "base/memory/ref_counted.h"
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
               scoped_refptr<BraveTooltipDelegate> delegate);

  // Creates a copy of the |other| tooltip. The delegate, if any,
  // will be identical for both tooltip instances. The |id| of
  // the tooltip will be replaced by the given value
  BraveTooltip(const std::string& id, const BraveTooltip& other);

  // Creates a copy of the |other| tooltip. The delegate will be
  // replaced by |delegate|
  BraveTooltip(scoped_refptr<BraveTooltipDelegate> delegate,
               const BraveTooltip& other);

  // Creates a copy of the |other| tooltip. The delegate, if any, will
  // be identical for both tooltip instances
  BraveTooltip(const BraveTooltip& other);

  BraveTooltip& operator=(const BraveTooltip& other);

  virtual ~BraveTooltip();

  const std::string& id() const { return id_; }

  const BraveTooltipAttributes& attributes() const { return attributes_; }
  void set_attributes(const BraveTooltipAttributes& attributes) {
    attributes_ = attributes;
  }

  std::u16string accessible_name() const;

  BraveTooltipDelegate* delegate() const { return delegate_.get(); }

  void set_delegate(scoped_refptr<BraveTooltipDelegate> delegate) {
    DCHECK(!delegate_);
    delegate_ = std::move(delegate);
  }

 protected:
  std::string id_;

  std::u16string title_;
  std::u16string body_;

  BraveTooltipAttributes attributes_;

 private:
  // A proxy object that allows access back to the JavaScript object that
  // represents the tooltip, for firing events
  scoped_refptr<BraveTooltipDelegate> delegate_;
};

}  // namespace brave_tooltips

#endif  // BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_H_
