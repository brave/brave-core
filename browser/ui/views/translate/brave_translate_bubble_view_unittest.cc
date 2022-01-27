/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/views/translate/brave_translate_bubble_view.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/label_button.h"

namespace {

class MockTranslateBubbleModel : public TranslateBubbleModel {
 public:
  explicit MockTranslateBubbleModel(TranslateBubbleModel::ViewState view_state)
      : view_state_transition_(view_state),
        error_type_(translate::TranslateErrors::NONE),
        original_language_index_(0),
        target_language_index_(1),
        never_translate_language_(false),
        never_translate_site_(false),
        should_always_translate_(false),
        always_translate_checked_(false),
        set_always_translate_called_count_(0),
        translate_called_(false),
        revert_translation_called_(false),
        translation_declined_(false),
        original_language_index_on_translation_(-1),
        target_language_index_on_translation_(-1),
        can_add_site_to_never_prompt_list_(true) {}

  TranslateBubbleModel::ViewState GetViewState() const override {
    return view_state_transition_.view_state();
  }

  void SetViewState(TranslateBubbleModel::ViewState view_state) override {
    view_state_transition_.SetViewState(view_state);
  }

  void ShowError(translate::TranslateErrors::Type error_type) override {
    error_type_ = error_type;
  }

  void GoBackFromAdvanced() override {
    view_state_transition_.GoBackFromAdvanced();
  }

  int GetNumberOfSourceLanguages() const override { return 1000; }

  int GetNumberOfTargetLanguages() const override { return 1000; }

  std::u16string GetSourceLanguageNameAt(int index) const override {
    return u"English";
  }

  std::u16string GetTargetLanguageNameAt(int index) const override {
    return u"Spanish";
  }

  std::string GetSourceLanguageCode() const override {
    return std::string("en-US");
  }

  int GetSourceLanguageIndex() const override {
    return original_language_index_;
  }

  void UpdateSourceLanguageIndex(int index) override {
    original_language_index_ = index;
  }

  int GetTargetLanguageIndex() const override { return target_language_index_; }

  void UpdateTargetLanguageIndex(int index) override {
    target_language_index_ = index;
  }

  void DeclineTranslation() override { translation_declined_ = true; }

  bool ShouldNeverTranslateLanguage() override {
    return never_translate_language_;
  }

  void SetNeverTranslateLanguage(bool value) override {
    never_translate_language_ = value;
  }

  bool ShouldNeverTranslateSite() override { return never_translate_site_; }

  void SetNeverTranslateSite(bool value) override {
    never_translate_site_ = value;
  }

  bool ShouldAlwaysTranslateBeCheckedByDefault() const override {
    return always_translate_checked_;
  }

  bool ShouldAlwaysTranslate() const override {
    return should_always_translate_;
  }

  bool ShouldShowAlwaysTranslateShortcut() const override { return false; }

  void SetAlwaysTranslate(bool value) override {
    should_always_translate_ = value;
    set_always_translate_called_count_++;
  }

  void Translate() override {
    translate_called_ = true;
    original_language_index_on_translation_ = original_language_index_;
    target_language_index_on_translation_ = target_language_index_;
  }

  void RevertTranslation() override { revert_translation_called_ = true; }

  void OnBubbleClosing() override {}

  bool IsPageTranslatedInCurrentLanguages() const override {
    return original_language_index_on_translation_ ==
               original_language_index_ &&
           target_language_index_on_translation_ == target_language_index_;
  }

  bool CanAddSiteToNeverPromptList() override {
    return can_add_site_to_never_prompt_list_;
  }

  void ReportUIInteraction(translate::UIInteraction ui_interaction) override {}

  void SetCanAddSiteToNeverPromptList(bool value) {
    can_add_site_to_never_prompt_list_ = value;
  }

  TranslateBubbleViewStateTransition view_state_transition_;
  translate::TranslateErrors::Type error_type_;
  int original_language_index_;
  int target_language_index_;
  bool never_translate_language_;
  bool never_translate_site_;
  bool should_always_translate_;
  bool always_translate_checked_;
  int set_always_translate_called_count_;
  bool translate_called_;
  bool revert_translation_called_;
  bool translation_declined_;
  int original_language_index_on_translation_;
  int target_language_index_on_translation_;
  bool can_add_site_to_never_prompt_list_;
};

class MockBraveTranslateBubbleView : public BraveTranslateBubbleView {
 public:
  MockBraveTranslateBubbleView(views::View* anchor_view,
                               std::unique_ptr<TranslateBubbleModel> model,
                               translate::TranslateErrors::Type error_type,
                               content::WebContents* web_contents)
    : BraveTranslateBubbleView(anchor_view,
                               std::move(model),
                               error_type,
                               web_contents)
    , install_google_translate_called_(false) {
  }

  ~MockBraveTranslateBubbleView() override {
  }

  bool install_google_translate_called() {
    return install_google_translate_called_;
  }

 protected:
  void InstallGoogleTranslate() override {
    install_google_translate_called_ = true;
  }

 private:
  bool install_google_translate_called_;
};

}  // namespace

class BraveTranslateBubbleViewTest : public ChromeViewsTestBase {
 public:
  BraveTranslateBubbleViewTest() {}

 protected:
  void SetUp() override {
    ChromeViewsTestBase::SetUp();

    // The bubble needs the parent as an anchor.
    views::Widget::InitParams params =
      CreateParams(views::Widget::InitParams::TYPE_WINDOW);
    params.ownership =
      views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;

    anchor_widget_.reset(new views::Widget());
    anchor_widget_->Init(std::move(params));
    anchor_widget_->Show();

    mock_model_ = new MockTranslateBubbleModel(
        TranslateBubbleModel::VIEW_STATE_BEFORE_TRANSLATE);
  }

  void CreateAndShowBubble() {
    std::unique_ptr<TranslateBubbleModel> model(mock_model_);
    bubble_ = new MockBraveTranslateBubbleView(
        anchor_widget_->GetContentsView(),
        std::move(model),
        translate::TranslateErrors::NONE, NULL);
    views::BubbleDialogDelegateView::CreateBubble(bubble_)->Show();
  }

  void PressButton(TranslateBubbleView::ButtonID id) {
    views::LabelButton button(views::Button::PressedCallback(), u"dummy");
    button.SetID(id);

    bubble_->ButtonPressed(id);
  }

  void TearDown() override {
    bubble_->GetWidget()->CloseNow();
    anchor_widget_.reset();

    ChromeViewsTestBase::TearDown();
  }

  std::unique_ptr<views::Widget> anchor_widget_;
  raw_ptr<MockTranslateBubbleModel> mock_model_ = nullptr;
  raw_ptr<MockBraveTranslateBubbleView> bubble_ = nullptr;
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(BraveTranslateBubbleViewTest, BraveBeforeTranslateView) {
  CreateAndShowBubble();
  views::Button* accept_button = static_cast<views::Button*>(
      bubble_->GetViewByID(TranslateBubbleView::BUTTON_ID_DONE));
  EXPECT_TRUE(accept_button);
  views::Button* cancel_button = static_cast<views::Button*>(
      bubble_->GetViewByID(TranslateBubbleView::BUTTON_ID_CLOSE));
  EXPECT_TRUE(cancel_button);
}

TEST_F(BraveTranslateBubbleViewTest, TranslateButton) {
  CreateAndShowBubble();
  EXPECT_FALSE(mock_model_->translate_called_);
  EXPECT_FALSE(bubble_->install_google_translate_called());

  // Press the "Translate" button.
  PressButton(TranslateBubbleView::BUTTON_ID_DONE);
  EXPECT_FALSE(mock_model_->translate_called_);
  EXPECT_TRUE(bubble_->install_google_translate_called());
}

TEST_F(BraveTranslateBubbleViewTest, CancelButton) {
  CreateAndShowBubble();
  EXPECT_FALSE(bubble_->GetWidget()->IsClosed());

  // Press the "Cancel" button.
  PressButton(TranslateBubbleView::BUTTON_ID_CLOSE);
  EXPECT_TRUE(bubble_->GetWidget()->IsClosed());
}

TEST_F(BraveTranslateBubbleViewTest, ReturnKey) {
  CreateAndShowBubble();
  EXPECT_FALSE(mock_model_->translate_called_);
  EXPECT_FALSE(bubble_->install_google_translate_called());

  // Press ReturnKey
  bubble_->AcceleratorPressed(ui::Accelerator(ui::VKEY_RETURN, ui::EF_NONE));
  EXPECT_FALSE(mock_model_->translate_called_);
  EXPECT_TRUE(bubble_->install_google_translate_called());
}
