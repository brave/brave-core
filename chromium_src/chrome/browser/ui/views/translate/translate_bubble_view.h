/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TRANSLATE_TRANSLATE_BUBBLE_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TRANSLATE_TRANSLATE_BUBBLE_VIEW_H_

#define BRAVE_TRANSLATE_BUBBLE_VIEW_H_ \
  private: \
    friend class BraveTranslateBubbleView; \
    friend class BraveTranslateBubbleViewTest; \
    FRIEND_TEST_ALL_PREFIXES(BraveTranslateBubbleViewTest, \
                             BraveBeforeTranslateView); \
    FRIEND_TEST_ALL_PREFIXES(BraveTranslateBubbleViewTest, TranslateButton); \
    FRIEND_TEST_ALL_PREFIXES(BraveTranslateBubbleViewTest, CancelButton); \
  public:
// define BRAVE_TRANSLATE_BUBBLE_VIEW_H_

#define TranslateBubbleView ChromiumTranslateBubbleView
#include "../../../../../../../chrome/browser/ui/views/translate/translate_bubble_view.h"
#undef TranslateBubbleView

#define TranslateBubbleView BraveGoTranslateBubbleView

class BraveGoTranslateBubbleView : public ChromiumTranslateBubbleView {
 public:
  BraveGoTranslateBubbleView(views::View* anchor_view,
                             std::unique_ptr<TranslateBubbleModel> model,
                             translate::TranslateErrors::Type error_type,
                             content::WebContents* web_contents);

  ~BraveGoTranslateBubbleView() override;

  // Shows the Translate bubble. Returns the newly created bubble's Widget or
  // nullptr in cases when the bubble already exists or when the bubble is not
  // created.
  //
  // |is_user_gesture| is true when the bubble is shown on the user's deliberate
  // action.
  static views::Widget* ShowBubble(views::View* anchor_view,
                                   views::Button* highlighted_button,
                                   content::WebContents* web_contents,
                                   translate::TranslateStep step,
                                   const std::string& source_language,
                                   const std::string& target_language,
                                   translate::TranslateErrors::Type error_type,
                                   DisplayReason reason);

  void Init() override;

 private:
  friend class BraveTranslateBubbleViewTest;

  // Handles the reset button in advanced view under Tab UI.
  void ResetLanguage();

  // Creates the view used before/during/after translate.
  std::unique_ptr<views::View> CreateView();

  // Creates source language label and combobox for Tab UI advanced view. Caller
  // takes ownership of the returned view.
  std::unique_ptr<views::View> CreateViewAdvancedSource();

  // Creates source language label and combobox for Tab UI advanced view. Caller
  // takes ownership of the returned view.
  std::unique_ptr<views::View> CreateViewAdvancedTarget();

  // Creates the 'advanced' view to show source/target language combobox. Caller
  // takes ownership of the returned view.
  std::unique_ptr<views::View> CreateViewAdvanced(
      std::unique_ptr<views::Combobox> combobox,
      std::unique_ptr<views::Label> language_title_label,
      std::unique_ptr<views::Button> advanced_reset_button,
      std::unique_ptr<views::Button> advanced_done_button,
      std::unique_ptr<views::Checkbox> advanced_always_translate_checkbox);

  void DisableOfferTranslatePref();

  void ButtonPressed(ButtonID button_id);

  // Remove this. As we replace |translate_view_|, we should destroy after
  // replacing it. However, its child view(|tabbed_pane_|) is still referenced
  // from TranslateBubbleView. Keep to prevent leak.
  std::unique_ptr<views::View> removed_translate_view_;

  DISALLOW_COPY_AND_ASSIGN(BraveGoTranslateBubbleView);
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TRANSLATE_TRANSLATE_BUBBLE_VIEW_H_
