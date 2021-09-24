/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/translate/brave_translate_bubble_view.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/grit/generated_resources.h"

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
#undef IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE
#define IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE \
  IDS_BRAVE_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE
#elif BUILDFLAG(ENABLE_BRAVE_TRANSLATE_EXTENSION)
#undef IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE
#define IDS_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_TITLE \
  IDS_BRAVE_TRANSLATE_BUBBLE_BEFORE_TRANSLATE_INSTALL_TITLE
#endif

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_EXTENSION)
#define BRAVE_TRANSLATE_BUBBLE_VIEW_ BraveTranslateBubbleView
#else
#define BRAVE_TRANSLATE_BUBBLE_VIEW_ TranslateBubbleView
#endif

#include "chrome/browser/ui/views/translate/translate_bubble_view.h"

#undef TranslateBubbleView
#define TranslateBubbleView ChromiumTranslateBubbleView
#include "../../../../../../../chrome/browser/ui/views/translate/translate_bubble_view.cc"
#undef TranslateBubbleView
#define TranslateBubbleView BraveGoTranslateBubbleView

BraveGoTranslateBubbleView::BraveGoTranslateBubbleView(
    views::View* anchor_view,
    std::unique_ptr<TranslateBubbleModel> model,
    translate::TranslateErrors::Type error_type,
    content::WebContents* web_contents)
    : ChromiumTranslateBubbleView(anchor_view,
                                  std::move(model),
                                  error_type,
                                  web_contents) {
}

BraveGoTranslateBubbleView::~BraveGoTranslateBubbleView() {}

// static
views::Widget* BraveGoTranslateBubbleView::ShowBubble(
    views::View* anchor_view,
    views::Button* highlighted_button,
    content::WebContents* web_contents,
    translate::TranslateStep step,
    const std::string& source_language,
    const std::string& target_language,
    translate::TranslateErrors::Type error_type,
    LocationBarBubbleDelegateView::DisplayReason reason) {
  if (translate_bubble_view_) {
    // When the user reads the advanced setting panel, the bubble should not be
    // changed because they are focusing on the bubble.
    if (translate_bubble_view_->web_contents() == web_contents &&
        (translate_bubble_view_->model()->GetViewState() ==
             TranslateBubbleModel::VIEW_STATE_SOURCE_LANGUAGE ||
         translate_bubble_view_->model()->GetViewState() ==
             TranslateBubbleModel::VIEW_STATE_TARGET_LANGUAGE)) {
      return nullptr;
    }
    if (step != translate::TRANSLATE_STEP_TRANSLATE_ERROR) {
      TranslateBubbleModel::ViewState state =
          TranslateBubbleModelImpl::TranslateStepToViewState(step);
      translate_bubble_view_->SwitchView(state);
    } else {
      translate_bubble_view_->SwitchToErrorView(error_type);
    }
    return nullptr;
  } else {
    if (step == translate::TRANSLATE_STEP_AFTER_TRANSLATE &&
        reason == AUTOMATIC) {
      return nullptr;
    }
  }
  std::unique_ptr<translate::TranslateUIDelegate> ui_delegate(
      new translate::TranslateUIDelegate(
          ChromeTranslateClient::GetManagerFromWebContents(web_contents)
              ->GetWeakPtr(),
          source_language, target_language));
  std::unique_ptr<TranslateBubbleModel> model(
      new TranslateBubbleModelImpl(step, std::move(ui_delegate)));
  TranslateBubbleView* view = new BRAVE_TRANSLATE_BUBBLE_VIEW_(
      anchor_view, std::move(model), error_type, web_contents);

  if (highlighted_button)
    view->SetHighlightedButton(highlighted_button);
  views::Widget* bubble_widget =
      views::BubbleDialogDelegateView::CreateBubble(view);

  // TAB UI has the same view throughout. Select the right tab based on |step|
  // upon initialization.
  if (step != translate::TRANSLATE_STEP_TRANSLATE_ERROR) {
    TranslateBubbleModel::ViewState state =
        TranslateBubbleModelImpl::TranslateStepToViewState(step);
    translate_bubble_view_->SwitchView(state);
  } else {
    translate_bubble_view_->SwitchToErrorView(error_type);
  }

  // |allow_refocus_alert| is set to false because translate bubble does not
  // have an additional screen reader alert instructing the user to use a
  // hotkey combination to focus the bubble.
  view->ShowForReason(reason, false);
  translate::ReportUiAction(translate::BUBBLE_SHOWN);

  ChromeTranslateClient::GetManagerFromWebContents(web_contents)
      ->GetActiveTranslateMetricsLogger()
      ->LogUIChange(true);

  return bubble_widget;
}

void BraveGoTranslateBubbleView::Init() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  should_always_translate_ = model_->ShouldAlwaysTranslate();
  should_never_translate_language_ = model_->ShouldNeverTranslateLanguage();
  should_never_translate_site_ = model_->ShouldNeverTranslateSite();
  translate_view_ = AddChildView(CreateView());
  advanced_view_source_ = AddChildView(CreateViewAdvancedSource());
  advanced_view_target_ = AddChildView(CreateViewAdvancedTarget());
  error_view_ = AddChildView(CreateViewError());

  AddAccelerator(ui::Accelerator(ui::VKEY_RETURN, ui::EF_NONE));

  UpdateChildVisibilities();

  if (GetViewState() == TranslateBubbleModel::VIEW_STATE_ERROR)
    model_->ShowError(error_type_);
}

void BraveGoTranslateBubbleView::ResetLanguage() {
  if (GetViewState() == TranslateBubbleModel::VIEW_STATE_SOURCE_LANGUAGE) {
    if (source_language_combobox_->GetSelectedIndex() ==
        previous_source_language_index_) {
      return;
    }
    source_language_combobox_->SetSelectedIndex(
        previous_source_language_index_);
    model_->UpdateSourceLanguageIndex(
        source_language_combobox_->GetSelectedIndex());
  } else {
    if (target_language_combobox_->GetSelectedIndex() ==
        previous_target_language_index_) {
      return;
    }
    target_language_combobox_->SetSelectedIndex(
        previous_target_language_index_);
    model_->UpdateTargetLanguageIndex(
        target_language_combobox_->GetSelectedIndex());
  }
  UpdateAdvancedView();
}

std::unique_ptr<views::View> BraveGoTranslateBubbleView::CreateView() {
  std::u16string source_language_name;
  std::u16string target_language_name;
  UpdateLanguageNames(&source_language_name, &target_language_name);

  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();

  auto view = std::make_unique<views::View>();
  view->SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kVertical);

  auto inner_view = std::make_unique<views::View>();
  inner_view->SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal);
  auto* horizontal_view = view->AddChildView(std::move(inner_view));

  views::View* icon = nullptr;
  // if (!UseGoogleTranslateBranding()) {
  //   icon = horizontal_view->AddChildView(CreateTranslateIcon());
  // }

  // Tabbed pane for language selection. Can't use unique_ptr because
  // tabs have to be added after the tabbed_pane is added to the parent,
  // when we release ownership of the unique_ptr.
  auto tabbed_pane = std::make_unique<views::TabbedPane>();
  tabbed_pane_ = horizontal_view->AddChildView(std::move(tabbed_pane));

  // NOTE: Panes must be added after |tabbed_pane| has been added to its
  // parent.
  tabbed_pane_->AddTab(source_language_name, CreateEmptyPane());
  tabbed_pane_->AddTab(target_language_name, CreateEmptyPane());
  tabbed_pane_->GetTabAt(0)->SetProperty(views::kElementIdentifierKey,
                                         kSourceLanguageTab);
  tabbed_pane_->GetTabAt(1)->SetProperty(views::kElementIdentifierKey,
                                         kTargetLanguageTab);
  tabbed_pane_->GetTabAt(0)->SetBorder(
      views::CreateEmptyBorder(gfx::Insets(2, 20)));
  tabbed_pane_->GetTabAt(1)->SetBorder(
      views::CreateEmptyBorder(gfx::Insets(2, 20)));
  tabbed_pane_->set_listener(this);

  auto* padding_view =
      horizontal_view->AddChildView(std::make_unique<views::View>());
  auto* options_menu = horizontal_view->AddChildView(CreateOptionsMenuButton());
  horizontal_view->AddChildView(CreateCloseButton());

  // Don't show the the always translate checkbox if the source language is
  // unknown.
  auto source_language_code = model_->GetSourceLanguageCode();
  if (model_->ShouldShowAlwaysTranslateShortcut() &&
      source_language_code != translate::kUnknownLanguageCode) {
    auto before_always_translate_checkbox = std::make_unique<views::Checkbox>(
        l10n_util::GetStringFUTF16(
            IDS_TRANSLATE_BUBBLE_ALWAYS_TRANSLATE_LANG,
            model_->GetSourceLanguageNameAt(model_->GetSourceLanguageIndex())),
        base::BindRepeating(&TranslateBubbleView::AlwaysTranslatePressed,
                            base::Unretained(this)));
    before_always_translate_checkbox->SetID(BUTTON_ID_ALWAYS_TRANSLATE);
    always_translate_checkbox_ =
        view->AddChildView(std::move(before_always_translate_checkbox));
  }

  if (icon) {
    icon->SetProperty(
        views::kMarginsKey,
        gfx::Insets(0, 0, 0,
                    provider->GetDistanceMetric(
                        views::DISTANCE_RELATED_BUTTON_HORIZONTAL)));
  }
  tabbed_pane_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToMinimum,
                               views::MaximumFlexSizeRule::kPreferred));
  padding_view->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kUnbounded)
          .WithOrder(2));
  options_menu->SetProperty(views::kElementIdentifierKey, kOptionsMenuButton);
  options_menu->SetProperty(
      views::kMarginsKey,
      gfx::Insets(0, provider->GetDistanceMetric(
                         views::DISTANCE_RELATED_BUTTON_HORIZONTAL)));
  if (always_translate_checkbox_) {
    horizontal_view->SetProperty(
        views::kMarginsKey,
        gfx::Insets(0, 0,
                    provider->GetDistanceMetric(
                        views::DISTANCE_RELATED_CONTROL_VERTICAL),
                    0));
    always_translate_checkbox_->SetProperty(views::kMarginsKey,
                                            gfx::Insets(2, 0));
  }

  return view;
}

std::unique_ptr<views::View> BraveGoTranslateBubbleView::CreateViewAdvanced(
    std::unique_ptr<views::Combobox> combobox,
    std::unique_ptr<views::Label> language_title_label,
    std::unique_ptr<views::Button> advanced_reset_button,
    std::unique_ptr<views::Button> advanced_done_button,
    std::unique_ptr<views::Checkbox> advanced_always_translate_checkbox) {
  auto view = std::make_unique<AdvancedViewContainer>();
  views::GridLayout* layout =
      view->SetLayoutManager(std::make_unique<views::GridLayout>());

  std::unique_ptr<views::ImageView> language_icon = CreateTranslateIcon();

  enum {
    COLUMN_SET_ID_TITLE,
    COLUMN_SET_ID_LANGUAGES,
    COLUMN_SET_ID_ALWAYS_CHECKBOX,
    COLUMN_SET_ID_BUTTONS
  };

  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();

  views::ColumnSet* cs = layout->AddColumnSet(COLUMN_SET_ID_TITLE);
  // if (!UseGoogleTranslateBranding()) {
  //   cs->AddColumn(views::GridLayout::TRAILING, views::GridLayout::CENTER,
  //                 views::GridLayout::kFixedSize,
  //                 views::GridLayout::ColumnSize::kUsePreferred, 0, 0);
  //   cs->AddPaddingColumn(views::GridLayout::kFixedSize,
  //                        provider->GetDistanceMetric(
  //                            views::DISTANCE_RELATED_CONTROL_HORIZONTAL));
  // }
  cs->AddColumn(views::GridLayout::FILL, views::GridLayout::CENTER,
                views::GridLayout::kFixedSize,
                views::GridLayout::ColumnSize::kUsePreferred, 0, 0);
  cs->AddPaddingColumn(1, provider->GetDistanceMetric(
                              views::DISTANCE_RELATED_CONTROL_HORIZONTAL) *
                              4);
  cs->AddColumn(views::GridLayout::TRAILING, views::GridLayout::LEADING,
                views::GridLayout::kFixedSize,
                views::GridLayout::ColumnSize::kUsePreferred, 0, 0);

  cs = layout->AddColumnSet(COLUMN_SET_ID_LANGUAGES);

  // if (!UseGoogleTranslateBranding()) {
  //   cs->AddPaddingColumn(views::GridLayout::kFixedSize,
  //                        language_icon->CalculatePreferredSize().width());
  //   cs->AddPaddingColumn(views::GridLayout::kFixedSize,
  //                        provider->GetDistanceMetric(
  //                            views::DISTANCE_RELATED_CONTROL_HORIZONTAL));
  //   cs->AddColumn(views::GridLayout::FILL, views::GridLayout::CENTER, 1,
  //                 views::GridLayout::ColumnSize::kUsePreferred, 0, 0);
  // } else {
    cs->AddColumn(views::GridLayout::FILL, views::GridLayout::CENTER, 1,
                  views::GridLayout::ColumnSize::kUsePreferred, 0, 0);
  // }
  cs->AddPaddingColumn(
      views::GridLayout::kFixedSize,
      provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_HORIZONTAL));

  cs = layout->AddColumnSet(COLUMN_SET_ID_ALWAYS_CHECKBOX);
  // if (!UseGoogleTranslateBranding()) {
  //   cs->AddPaddingColumn(views::GridLayout::kFixedSize,
  //                        language_icon->CalculatePreferredSize().width());
  //   cs->AddPaddingColumn(views::GridLayout::kFixedSize,
  //                        provider->GetDistanceMetric(
  //                            views::DISTANCE_RELATED_CONTROL_HORIZONTAL));
  //   cs->AddColumn(views::GridLayout::TRAILING, views::GridLayout::CENTER,
  //                 views::GridLayout::kFixedSize,
  //                 views::GridLayout::ColumnSize::kUsePreferred, 0, 0);
  // } else {
    cs->AddColumn(views::GridLayout::TRAILING, views::GridLayout::CENTER,
                  views::GridLayout::kFixedSize,
                  views::GridLayout::ColumnSize::kUsePreferred, 0, 0);
  // }

  cs = layout->AddColumnSet(COLUMN_SET_ID_BUTTONS);
  cs->AddColumn(views::GridLayout::LEADING, views::GridLayout::CENTER,
                views::GridLayout::kFixedSize,
                views::GridLayout::ColumnSize::kUsePreferred, 0, 0);
  cs->AddPaddingColumn(
      1.0, provider->GetDistanceMetric(DISTANCE_UNRELATED_CONTROL_HORIZONTAL));
  cs->AddColumn(views::GridLayout::LEADING, views::GridLayout::CENTER,
                views::GridLayout::kFixedSize,
                views::GridLayout::ColumnSize::kUsePreferred, 0, 0);
  cs->AddPaddingColumn(
      views::GridLayout::kFixedSize,
      provider->GetDistanceMetric(views::DISTANCE_RELATED_BUTTON_HORIZONTAL));
  cs->AddColumn(views::GridLayout::LEADING, views::GridLayout::CENTER,
                views::GridLayout::kFixedSize,
                views::GridLayout::ColumnSize::kUsePreferred, 0, 0);
  cs->AddPaddingColumn(
      views::GridLayout::kFixedSize,
      provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_HORIZONTAL));

  layout->StartRow(views::GridLayout::kFixedSize, COLUMN_SET_ID_TITLE);
  // if (!UseGoogleTranslateBranding()) {
  //   // If the bottom branding isn't showing, display the leading translate
  //   // icon otherwise it's not obvious what the bubble is about. This should
  //   // only happen on non-Chrome-branded builds.
  //   layout->AddView(std::move(language_icon));
  // }
  const int vertical_spacing =
      provider->GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_VERTICAL);
  language_title_label->SetLineHeight(vertical_spacing * 5);
  layout->AddView(std::move(language_title_label));
  layout->AddView(CreateCloseButton());

  layout->AddPaddingRow(views::GridLayout::kFixedSize, vertical_spacing);

  layout->StartRow(views::GridLayout::kFixedSize, COLUMN_SET_ID_LANGUAGES);
  layout->AddView(std::move(combobox));

  if (advanced_always_translate_checkbox) {
    layout->AddPaddingRow(views::GridLayout::kFixedSize, vertical_spacing);
    layout->StartRow(views::GridLayout::kFixedSize,
                     COLUMN_SET_ID_ALWAYS_CHECKBOX);
    advanced_always_translate_checkbox_ =
        layout->AddView(std::move(advanced_always_translate_checkbox));
    layout->AddPaddingRow(views::GridLayout::kFixedSize, vertical_spacing * 2);
  } else {
    layout->AddPaddingRow(views::GridLayout::kFixedSize, vertical_spacing * 3);
  }

  layout->StartRow(views::GridLayout::kFixedSize, COLUMN_SET_ID_BUTTONS);
  layout->SkipColumns(1);

  layout->AddView(std::move(advanced_reset_button));
  layout->AddView(std::move(advanced_done_button));

  UpdateAdvancedView();

  return view;
}

std::unique_ptr<views::View> BraveGoTranslateBubbleView::CreateViewAdvancedSource() {
  // Bubble title
  std::unique_ptr<views::Label> source_language_title_label =
      std::make_unique<views::Label>(
          l10n_util::GetStringUTF16(IDS_TRANSLATE_BUBBLE_ADVANCED_SOURCE),
          views::style::CONTEXT_DIALOG_TITLE);

  // Language icon
  int source_default_index = model_->GetSourceLanguageIndex();
  source_language_combobox_model_ =
      std::make_unique<SourceLanguageComboboxModel>(source_default_index,
                                                    model_.get());

  // Ideally all child view elements shall be created using unique_ptr.
  // Using normal pointer for compatibility with existing code.
  auto source_language_combobox =
      std::make_unique<views::Combobox>(source_language_combobox_model_.get());
  source_language_combobox->SetProperty(views::kElementIdentifierKey,
                                        kSourceLanguageCombobox);

  // In an incognito window or when the source language is unknown, "Always
  // translate" checkbox shouldn't be shown.
  std::unique_ptr<views::Checkbox> advanced_always_translate_checkbox;
  auto source_language_code = model_->GetSourceLanguageCode();
  if (!is_in_incognito_window_ &&
      source_language_code != translate::kUnknownLanguageCode) {
    advanced_always_translate_checkbox = std::make_unique<views::Checkbox>(
        l10n_util::GetStringUTF16(IDS_TRANSLATE_BUBBLE_ALWAYS),
        base::BindRepeating(&TranslateBubbleView::AlwaysTranslatePressed,
                            base::Unretained(this)));
    advanced_always_translate_checkbox->SetID(BUTTON_ID_ALWAYS_TRANSLATE);
  }

  source_language_combobox->SetCallback(base::BindRepeating(
      &TranslateBubbleView::SourceLanguageChanged, base::Unretained(this)));
  source_language_combobox->SetAccessibleName(l10n_util::GetStringUTF16(
      IDS_TRANSLATE_BUBBLE_SOURCE_LANG_COMBOBOX_ACCNAME));
  source_language_combobox_ = source_language_combobox.get();

  auto advanced_reset_button = std::make_unique<views::MdTextButton>(
      base::BindRepeating(&TranslateBubbleView::ResetLanguage,
                          base::Unretained(this)),
      l10n_util::GetStringUTF16(IDS_TRANSLATE_BUBBLE_RESET));
  advanced_reset_button->SetID(BUTTON_ID_RESET);
  advanced_reset_button_source_ = advanced_reset_button.get();

  auto advanced_done_button = std::make_unique<views::MdTextButton>(
      base::BindRepeating(&TranslateBubbleView::ConfirmAdvancedOptions,
                          base::Unretained(this)),
      l10n_util::GetStringUTF16(IDS_DONE));
  advanced_done_button->SetID(BUTTON_ID_DONE);
  advanced_done_button->SetIsDefault(true);
  advanced_done_button_source_ = advanced_done_button.get();
  advanced_done_button_source_->SetProperty(views::kElementIdentifierKey,
                                            kSourceLanguageDoneButton);

  return CreateViewAdvanced(std::move(source_language_combobox),
                            std::move(source_language_title_label),
                            std::move(advanced_reset_button),
                            std::move(advanced_done_button),
                            std::move(advanced_always_translate_checkbox));
}

std::unique_ptr<views::View> BraveGoTranslateBubbleView::CreateViewAdvancedTarget() {
  // Bubble title
  std::unique_ptr<views::Label> target_language_title_label =
      std::make_unique<views::Label>(
          l10n_util::GetStringUTF16(IDS_TRANSLATE_BUBBLE_ADVANCED_TARGET),
          views::style::CONTEXT_DIALOG_TITLE);

  int target_default_index = model_->GetTargetLanguageIndex();
  target_language_combobox_model_ =
      std::make_unique<TargetLanguageComboboxModel>(target_default_index,
                                                    model_.get());

  // Ideally all view components shall be created using unique_ptr.
  // Using normal pointer for compatibility with existing code.
  auto target_language_combobox =
      std::make_unique<views::Combobox>(target_language_combobox_model_.get());
  target_language_combobox->SetProperty(views::kElementIdentifierKey,
                                        kTargetLanguageCombobox);

  target_language_combobox->SetCallback(base::BindRepeating(
      &TranslateBubbleView::TargetLanguageChanged, base::Unretained(this)));
  target_language_combobox->SetAccessibleName(l10n_util::GetStringUTF16(
      IDS_TRANSLATE_BUBBLE_TARGET_LANG_COMBOBOX_ACCNAME));
  target_language_combobox_ = target_language_combobox.get();

  auto advanced_reset_button = std::make_unique<views::MdTextButton>(
      base::BindRepeating(&TranslateBubbleView::ResetLanguage,
                          base::Unretained(this)),
      l10n_util::GetStringUTF16(IDS_TRANSLATE_BUBBLE_RESET));
  advanced_reset_button->SetID(BUTTON_ID_RESET);
  advanced_reset_button_target_ = advanced_reset_button.get();

  auto advanced_done_button = std::make_unique<views::MdTextButton>(
      base::BindRepeating(&TranslateBubbleView::ConfirmAdvancedOptions,
                          base::Unretained(this)),
      l10n_util::GetStringUTF16(IDS_DONE));
  advanced_done_button->SetID(BUTTON_ID_DONE);
  advanced_done_button->SetIsDefault(true);
  advanced_done_button_target_ = advanced_done_button.get();
  advanced_done_button_target_->SetProperty(views::kElementIdentifierKey,
                                            kTargetLanguageDoneButton);

  return CreateViewAdvanced(std::move(target_language_combobox),
                            std::move(target_language_title_label),
                            std::move(advanced_reset_button),
                            std::move(advanced_done_button), nullptr);
}
