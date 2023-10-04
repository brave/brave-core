// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_UI_VIEWS_AUTOFILL_CONFIRM_AUTOCOMPLETE_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_AUTOFILL_CONFIRM_AUTOCOMPLETE_BUBBLE_VIEW_H_

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/autofill/confirm_autocomplete_bubble_controller.h"
#include "chrome/browser/ui/autofill/autofill_bubble_base.h"
#include "chrome/browser/ui/views/controls/obscurable_label_with_toggle_button.h"
#include "chrome/browser/ui/views/location_bar/location_bar_bubble_delegate_view.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/textfield/textfield_controller.h"

namespace content {
class WebContents;
}

namespace autofill {

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

  void Hide() override;

  void AddedToWidget() override;
  std::u16string GetWindowTitle() const override;
  void WindowClosing() override;

 protected:
  ~ConfirmAutocompleteBubbleView() override;

  ConfirmAutocompleteBubbleController* controller() const {
    return controller_;
  }

  void OnDialogAccepted();

 private:
  friend class SaveIbanBubbleViewFullFormBrowserTest;

  raw_ptr<ConfirmAutocompleteBubbleController> controller_;
};

}  // namespace autofill

#endif  // BRAVE_BROWSER_UI_VIEWS_AUTOFILL_CONFIRM_AUTOCOMPLETE_BUBBLE_VIEW_H_
