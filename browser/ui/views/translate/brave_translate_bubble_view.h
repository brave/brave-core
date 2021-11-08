/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TRANSLATE_BRAVE_TRANSLATE_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_TRANSLATE_BRAVE_TRANSLATE_BUBBLE_VIEW_H_

#include <memory>
#include <utility>

#include "chrome/browser/ui/views/translate/translate_bubble_view.h"

class BraveTranslateIconView;

// The purpose of this subclass is to repurpose the translate bubble to install
// google translate extension, this is only used when
// ENABLE_BRAVE_TRANSLATE_EXTENSION is true.
class BraveTranslateBubbleView : public TranslateBubbleView {
 public:
  BraveTranslateBubbleView(views::View* anchor_view,
                           std::unique_ptr<TranslateBubbleModel> model,
                           translate::TranslateErrors::Type error_type,
                           content::WebContents* web_contents);
  BraveTranslateBubbleView(const BraveTranslateBubbleView&) = delete;
  BraveTranslateBubbleView& operator=(const BraveTranslateBubbleView&) = delete;
  ~BraveTranslateBubbleView() override;

  // views::BubbleDialogDelegateView methods.
  void Init() override;

  // views::View methods.
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;

  // LocationBarBubbleDelegateView methods.
  bool ShouldShowWindowTitle() const override;

 protected:
  virtual void InstallGoogleTranslate();

 private:
  friend class BraveTranslateBubbleViewTest;
  std::unique_ptr<views::View> BraveCreateViewBeforeTranslate();
  void DisableOfferTranslatePref();
  void ButtonPressed(ButtonID button_id);

  int GetTitleBeforeTranslateTitle() override;

  // Remove this. As we replace |translate_view_|, we should destroy after
  // replacing it. However, its child view(|tabbed_pane_|) is still referenced
  // from TranslateBubbleView. Keep to prevent leak.
  std::unique_ptr<views::View> removed_translate_view_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TRANSLATE_BRAVE_TRANSLATE_BUBBLE_VIEW_H_
