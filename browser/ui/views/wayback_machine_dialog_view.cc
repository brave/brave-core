/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/wayback_machine_dialog_view.h"

#include <memory>
#include <string>

#include "base/functional/bind.h"
#include "brave/browser/ui/views/wayback_machine_fetch_button.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_tab_helper.h"
#include "brave/components/brave_wayback_machine/pref_names.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/window/dialog_client_view.h"

namespace {

BraveWaybackMachineTabHelper* GetTabHelper(content::WebContents* web_contents) {
  return BraveWaybackMachineTabHelper::FromWebContents(web_contents);
}

gfx::FontList GetFont(int font_size, gfx::Font::Weight weight) {
  gfx::FontList font_list;
  return font_list.DeriveWithSizeDelta(font_size - font_list.GetFontSize())
      .DeriveWithWeight(weight);
}

// Subclass for custom font.
class CustomMdTextButton : public views::MdTextButton {
  METADATA_HEADER(CustomMdTextButton, views::MdTextButton)
 public:
  using MdTextButton::MdTextButton;
  CustomMdTextButton(const CustomMdTextButton&) = delete;
  CustomMdTextButton& operator=(const CustomMdTextButton&) = delete;

  void SetFontSize(int size) {
    label()->SetFontList(GetFont(size, gfx::Font::Weight::SEMIBOLD));
  }
};

BEGIN_METADATA(CustomMdTextButton)
END_METADATA

// Subclass for custom font.
class DontAskAgainCheckbox : public views::Checkbox {
  METADATA_HEADER(DontAskAgainCheckbox, views::Checkbox)
 public:
  using views::Checkbox::Checkbox;
  ~DontAskAgainCheckbox() override = default;

  void SetFontList(const gfx::FontList& font_list) {
    label()->SetFontList(font_list);
  }
};

BEGIN_METADATA(DontAskAgainCheckbox)
END_METADATA

}  // namespace

namespace brave {

void ShowWaybackMachineWebModalDialog(content::WebContents* web_contents) {
  auto* tab_helper = GetTabHelper(web_contents);
  if (!tab_helper) {
    return;
  }

  // Close previous one if exists.
  if (gfx::NativeWindow previous_dialog = tab_helper->active_dialog()) {
    views::Widget::GetWidgetForNativeWindow(previous_dialog)
        ->CloseWithReason(views::Widget::ClosedReason::kUnspecified);
  }

  auto* widget = constrained_window::ShowWebModalDialogViews(
      new WaybackMachineDialogView(web_contents), web_contents);
  tab_helper->set_active_dialog(widget->GetNativeWindow());
}

}  // namespace brave

WaybackMachineDialogView::WaybackMachineDialogView(
    content::WebContents* web_contents)
    : web_contents_(web_contents),
      wayback_machine_url_fetcher_(
          this,
          web_contents_->GetBrowserContext()
              ->GetDefaultStoragePartition()
              ->GetURLLoaderFactoryForBrowserProcess()),
      pref_service_(
          user_prefs::UserPrefs::Get(web_contents_->GetBrowserContext())) {
  SetModalType(ui::MODAL_TYPE_CHILD);
  SetButtons(ui::DIALOG_BUTTON_NONE);

  // Unretained here is safe because this class is owned by widget.
  RegisterWindowWillCloseCallback(base::BindOnce(
      &WaybackMachineDialogView::OnWillCloseDialog, base::Unretained(this)));

  SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kVertical)
      .SetMainAxisAlignment(views::LayoutAlignment::kStart)
      .SetInteriorMargin(gfx::Insets::TLBR(0, 26, 26, 26));

  auto* label = CreateLabel(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_WAYBACK_MACHINE_INFOBAR_PAGE_MISSING_TEXT));
  views_visible_before_checking_.push_back(label);
  label->SetFontList(
      label->font_list().DeriveWithWeight(gfx::Font::Weight::BOLD));
  label->SetProperty(views::kMarginsKey, gfx::Insets::TLBR(0, 0, 10, 0));
  AddChildView(label);

  label = CreateLabel(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_WAYBACK_MACHINE_INFOBAR_ASK_ABOUT_CHECK_TEXT));
  views_visible_before_checking_.push_back(label);
  label->SetMultiLine(true);
  label->SetMaximumWidth(400);
  label->SetProperty(views::kMarginsKey, gfx::Insets::TLBR(0, 0, 10, 0));
  AddChildView(label);

  // Unretained is safe beaause this button is owned by this class.
  auto* dont_ask_again = AddChildView(std::make_unique<DontAskAgainCheckbox>(
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_BRAVE_WAYBACK_MACHINE_DONT_ASK_AGAIN_TEXT),
      base::BindRepeating(&WaybackMachineDialogView::OnCheckboxUpdated,
                          base::Unretained(this))));
  views_visible_before_checking_.push_back(dont_ask_again);

  // Use same font with label. Checkbox's default font size is a little bit
  // smaller than label.
  dont_ask_again->SetFontList(label->font_list());
  dont_ask_again->SetProperty(views::kMarginsKey,
                              gfx::Insets::TLBR(0, 0, 10, 0));
  dont_ask_again_ = dont_ask_again;

  label = CreateLabel(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_WAYBACK_MACHINE_INFOBAR_NOT_AVAILABLE_TEXT));
  views_visible_after_checking_.push_back(label);
  label->SetFontList(
      label->font_list().DeriveWithWeight(gfx::Font::Weight::BOLD));
  label->SetProperty(views::kMarginsKey, gfx::Insets::TLBR(0, 0, 40, 60));
  AddChildView(label);

  auto* button_row = AddChildView(std::make_unique<views::View>());
  button_row
      ->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          /*inside_border_insets*/ gfx::Insets(),
          /*between_child_spacing*/ 12,
          /*collapse_margins_spacing*/ true))
      ->set_main_axis_alignment(views::BoxLayout::MainAxisAlignment::kEnd);

  // Unretained is safe beaause this button is owned by this class.
  auto* no_thanks =
      button_row->AddChildView(std::make_unique<CustomMdTextButton>(
          views::Button::PressedCallback(base::BindRepeating(
              &WaybackMachineDialogView::OnCancel, base::Unretained(this)))));
  no_thanks->SetKind(views::MdTextButton::Kind::kQuaternary);
  no_thanks->SetText(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_WAYBACK_MACHINE_NO_THANKS_BUTTON_TEXT));
  no_thanks->SetFontSize(13);
  no_thanks->SetTooltipText(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_WAYBACK_MACHINE_NO_THANKS_BUTTON_TEXT));
  no_thanks_ = no_thanks;

  fetch_url_button_ = button_row->AddChildView(
      std::make_unique<WaybackMachineFetchButton>(base::BindRepeating(
          &WaybackMachineDialogView::OnFetchURLButtonPressed,
          base::Unretained(this))));
  views_visible_before_checking_.push_back(fetch_url_button_.get());

  // Unretained is safe beaause this button is owned by this class.
  auto* close = button_row->AddChildView(std::make_unique<CustomMdTextButton>(
      views::Button::PressedCallback(base::BindRepeating(
          &WaybackMachineDialogView::OnCancel, base::Unretained(this)))));
  views_visible_after_checking_.push_back(close);
  close->SetKind(views::MdTextButton::Kind::kPrimary);
  close->SetText(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_WAYBACK_MACHINE_CLOSE_BUTTON_TEXT));
  close->SetTooltipText(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_WAYBACK_MACHINE_CLOSE_BUTTON_TEXT));

  UpdateChildrenVisibility(true);
}

WaybackMachineDialogView::~WaybackMachineDialogView() = default;

views::Label* WaybackMachineDialogView::CreateLabel(
    const std::u16string& text) {
  views::Label* label =
      new views::Label(text, views::style::CONTEXT_DIALOG_BODY_TEXT);
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  return label;
}

void WaybackMachineDialogView::UpdateChildrenVisibility(
    bool show_before_checking_views) {
  for (views::View* view : views_visible_before_checking_) {
    view->SetVisible(show_before_checking_views);
  }
  for (views::View* view : views_visible_after_checking_) {
    view->SetVisible(!show_before_checking_views);
  }
}

void WaybackMachineDialogView::OnCheckboxUpdated() {
  pref_service_->SetBoolean(kBraveWaybackMachineEnabled,
                            !dont_ask_again_->GetChecked());
}

void WaybackMachineDialogView::OnFetchURLButtonPressed() {
  if (wayback_url_fetch_requested_) {
    return;
  }

  // We don't need to show no thanks button anymore after fetching starts.
  no_thanks_->SetVisible(false);
  wayback_url_fetch_requested_ = true;
  FetchWaybackURL();
}

void WaybackMachineDialogView::FetchWaybackURL() {
  fetch_url_button_->StartThrobber();
  wayback_machine_url_fetcher_.Fetch(web_contents_->GetVisibleURL());
}

void WaybackMachineDialogView::LoadURL(const GURL& url) {
  web_contents_->GetController().LoadURL(
      url, content::Referrer(), ui::PAGE_TRANSITION_LINK, std::string());
}

void WaybackMachineDialogView::OnWaybackURLFetched(
    const GURL& latest_wayback_url) {
  DCHECK(wayback_url_fetch_requested_);
  wayback_url_fetch_requested_ = false;

  fetch_url_button_->StopThrobber();

  if (latest_wayback_url.is_empty()) {
    UpdateDialogForWaybackNotAvailable();
    return;
  }

  LoadURL(latest_wayback_url);
  // After loading to archived url, don't need to show dialog anymore.
  GetWidget()->CloseWithReason(
      views::Widget::ClosedReason::kAcceptButtonClicked);
}

void WaybackMachineDialogView::UpdateDialogForWaybackNotAvailable() {
  UpdateChildrenVisibility(false);
  // Update widget's size as children's visibility is changed.
  GetWidget()->SetSize(GetDialogClientView()->GetPreferredSize());
}

void WaybackMachineDialogView::OnWillCloseDialog() {
  if (auto* tab_helper = GetTabHelper(web_contents_)) {
    tab_helper->set_active_dialog(nullptr);
  }
}

void WaybackMachineDialogView::OnCancel() {
  GetWidget()->CloseWithReason(
      views::Widget::ClosedReason::kCancelButtonClicked);
}

BEGIN_METADATA(WaybackMachineDialogView)
END_METADATA
