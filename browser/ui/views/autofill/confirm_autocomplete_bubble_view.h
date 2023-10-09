/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_AUTOFILL_CONFIRM_AUTOCOMPLETE_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_AUTOFILL_CONFIRM_AUTOCOMPLETE_BUBBLE_VIEW_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/autofill/autofill_bubble_base.h"
#include "chrome/browser/ui/views/location_bar/location_bar_bubble_delegate_view.h"

namespace content {
class WebContents;
}

namespace views {
class View;
}

namespace autofill {

class ConfirmAutocompleteBubbleController;

class ConfirmAutocompleteBubbleView : public AutofillBubbleBase,
                                      public LocationBarBubbleDelegateView {
 public:
  ConfirmAutocompleteBubbleView(
      views::View* anchor_view,
      content::WebContents* web_contents,
      ConfirmAutocompleteBubbleController* controller);

  ConfirmAutocompleteBubbleView(const ConfirmAutocompleteBubbleView&) = delete;
  ConfirmAutocompleteBubbleView& operator=(
      const ConfirmAutocompleteBubbleView&) = delete;

  void Show(DisplayReason reason);

  // AutofillBubbleBase:
  void Hide() override;
  // View:
  void AddedToWidget() override;
  // WidgetDelegate:
  std::u16string GetWindowTitle() const override;
  void WindowClosing() override;

 protected:
  ~ConfirmAutocompleteBubbleView() override;

  ConfirmAutocompleteBubbleController* controller() const {
    return controller_;
  }

  void OnDialogAccepted();

 private:
  raw_ptr<ConfirmAutocompleteBubbleController> controller_;
};

}  // namespace autofill

#endif  // BRAVE_BROWSER_UI_VIEWS_AUTOFILL_CONFIRM_AUTOCOMPLETE_BUBBLE_VIEW_H_
