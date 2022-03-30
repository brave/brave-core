/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/web_discovery_dialog_view.h"

#include <utility>

#include "base/bind.h"
#include "brave/browser/ui/browser_dialogs.h"
#include "brave/common/pref_names.h"
#include "brave/common/url_constants.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/window/dialog_client_view.h"
#include "url/gurl.h"

namespace brave {

void ShowWebDiscoveryDialog(Browser* browser, content::WebContents* contents) {
  auto* prefs = user_prefs::UserPrefs::Get(contents->GetBrowserContext());
  constrained_window::ShowWebModalDialogViews(
      new WebDiscoveryDialogView(browser, prefs), contents)
      ->Show();
}

}  // namespace brave

// For vertically aligned dialog buttons.
class WebDiscoveryDialogClientView : public views::DialogClientView {
 public:
  METADATA_HEADER(WebDiscoveryDialogClientView);
  using DialogClientView::DialogClientView;

  WebDiscoveryDialogClientView(const WebDiscoveryDialogClientView&) = delete;
  WebDiscoveryDialogClientView& operator=(const WebDiscoveryDialogClientView&) =
      delete;

  // views::DialogClientView overrides:
  void SetupLayout() override {
    views::DialogClientView::SetupLayout();

    SetupButtonsLayoutVertically();
  }
};

BEGIN_METADATA(WebDiscoveryDialogClientView, views::DialogClientView)
END_METADATA

WebDiscoveryDialogView::WebDiscoveryDialogView(Browser* browser,
                                               PrefService* prefs)
    : browser_(browser), prefs_(prefs) {
  set_should_ignore_snapping(true);
  SetModalType(ui::MODAL_TYPE_CHILD);
  SetButtonLabel(ui::DIALOG_BUTTON_OK,
                 brave_l10n::GetLocalizedResourceUTF16String(
                     IDS_WEB_DISCOVERY_DIALOG_OK_BUTTON_TEXT));
  SetButtonLabel(ui::DIALOG_BUTTON_CANCEL,
                 brave_l10n::GetLocalizedResourceUTF16String(
                     IDS_WEB_DISCOVERY_DIALOG_CANCEL_BUTTON_TEXT));

  SetAcceptCallback(base::BindOnce(
      &WebDiscoveryDialogView::OnAcceptButtonClicked, base::Unretained(this)));
  RegisterWindowClosingCallback(base::BindOnce(
      &WebDiscoveryDialogView::OnWindowClosing, base::Unretained(this)));

  CreateChildViews();
}

WebDiscoveryDialogView::~WebDiscoveryDialogView() = default;

void WebDiscoveryDialogView::CreateChildViews() {
  SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kVertical)
      .SetMainAxisAlignment(views::LayoutAlignment::kStart)
      .SetInteriorMargin(gfx::Insets::TLBR(0, 36, 24, 36));

  auto& bundle = ui::ResourceBundle::GetSharedInstance();
  AddChildView(std::make_unique<views::ImageView>(
      ui::ImageModel::FromImageSkia(*bundle.GetImageSkiaNamed(
          IDR_BRAVE_SEARCH_LOGO_IN_WEB_DISCOVERY_DIALOG))));

  // Use 15px font size for header text.
  int size_diff = 15 - views::Label::GetDefaultFontList().GetFontSize();
  views::Label::CustomFont header_font = {
      views::Label::GetDefaultFontList()
          .DeriveWithSizeDelta(size_diff)
          .DeriveWithWeight(gfx::Font::Weight::SEMIBOLD)};
  auto* header_label = AddChildView(std::make_unique<views::Label>(
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_WEB_DISCOVERY_DIALOG_HEADER_TEXT),
      header_font));
  header_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header_label->SetProperty(views::kMarginsKey, gfx::Insets::TLBR(22, 0, 0, 0));

  size_t learn_more_offset;
  const std::u16string learn_more_text =
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_WEB_DISCOVERY_DIALOG_CONTENTS_TEXT_LEARN_MORE_PART);
  const std::u16string contents_text =
      l10n_util::GetStringFUTF16(IDS_WEB_DISCOVERY_DIALOG_CONTENTS_TEXT,
                                 learn_more_text, &learn_more_offset);

  auto* contents_label = AddChildView(std::make_unique<views::StyledLabel>());
  contents_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  contents_label->SetText(contents_text);
  contents_label->SetProperty(views::kMarginsKey,
                              gfx::Insets::TLBR(8, 0, 25, 0));

  // Apply link style to learn more link text.
  views::StyledLabel::RangeStyleInfo learn_more_style =
      views::StyledLabel::RangeStyleInfo::CreateForLink(base::BindRepeating(
          &WebDiscoveryDialogView::OnLearnMoreClicked, base::Unretained(this)));
  contents_label->AddStyleRange(
      gfx::Range(learn_more_offset,
                 learn_more_offset + learn_more_text.length()),
      learn_more_style);

  // Use 14px font size for contents text.
  views::StyledLabel::RangeStyleInfo contents_default_style;
  size_diff = 14 - views::Label::GetDefaultFontList().GetFontSize();
  contents_default_style.custom_font =
      views::Label::GetDefaultFontList()
          .DeriveWithSizeDelta(size_diff)
          .DeriveWithWeight(gfx::Font::Weight::NORMAL);

  contents_label->AddStyleRange(gfx::Range(0, learn_more_offset),
                                contents_default_style);
  contents_label->SizeToFit(360);

  dont_ask_again_checkbox_ = AddChildView(std::make_unique<views::Checkbox>(
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_WEB_DISCOVERY_DIALOG_DONT_ASK_AGAIN_TEXT)));
}

views::ClientView* WebDiscoveryDialogView::CreateClientView(
    views::Widget* widget) {
  return new WebDiscoveryDialogClientView(widget,
                                          TransferOwnershipOfContentsView());
}

bool WebDiscoveryDialogView::ShouldShowCloseButton() const {
  return true;
}

void WebDiscoveryDialogView::OnAcceptButtonClicked() {
  prefs_->SetBoolean(kWebDiscoveryEnabled, true);
}

void WebDiscoveryDialogView::OnWindowClosing() {
  prefs_->SetBoolean(kDontAskEnableWebDiscovery,
                     dont_ask_again_checkbox_->GetChecked());
}

void WebDiscoveryDialogView::OnLearnMoreClicked() {
  auto* tab_model = browser_->tab_strip_model();
  const int active_index = tab_model->active_index();
  // Open link right next to the current tab.
  chrome::AddTabAt(browser_, GURL(kWebDiscoveryLearnMoreUrl), active_index + 1,
                   true);
}

BEGIN_METADATA(WebDiscoveryDialogView, views::DialogDelegateView)
END_METADATA
