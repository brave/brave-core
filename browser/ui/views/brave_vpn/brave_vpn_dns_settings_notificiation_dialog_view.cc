/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_vpn/brave_vpn_dns_settings_notificiation_dialog_view.h"

#include <memory>
#include <utility>

#include "brave/components/brave_vpn/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/link.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/box_layout.h"

namespace brave_vpn {

namespace {

constexpr char kBraveVPNLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/10864482160141";

constexpr int kChildSpacing = 16;
constexpr int kPadding = 24;
constexpr int kTopPadding = 32;
constexpr int kBottomPadding = 26;
constexpr int kDialogWidth = 600;

}  // namespace

// static
void BraveVpnDnsSettingsNotificiationDialogView::Show(Browser* browser) {
  auto* prefs = browser->profile()->GetPrefs();
  if (!prefs->GetBoolean(prefs::kBraveVPNShowNotificationDialog))
    return;
  // The dialog eats mouse events which results in the close button
  // getting stuck in the hover state. Reset the window controls to
  // prevent this.
  BrowserView::GetBrowserViewForBrowser(browser)
      ->GetWidget()
      ->non_client_view()
      ->ResetWindowControls();

  constrained_window::CreateBrowserModalDialogViews(
      new BraveVpnDnsSettingsNotificiationDialogView(browser),
      browser->window()->GetNativeWindow())
      ->Show();
}

BraveVpnDnsSettingsNotificiationDialogView::
    BraveVpnDnsSettingsNotificiationDialogView(Browser* browser)
    : browser_(browser), prefs_(browser->profile()->GetPrefs()) {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::TLBR(kTopPadding, kPadding, kBottomPadding, kPadding),
      kChildSpacing));
  SetButtons(ui::DIALOG_BUTTON_OK);
  SetButtonLabel(ui::DIALOG_BUTTON_OK,
                 l10n_util::GetStringUTF16(
                     IDS_BRAVE_VPN_DNS_SETTINGS_NOTIFICATION_DIALOG_OK_TEXT));

  RegisterWindowClosingCallback(
      base::BindOnce(&BraveVpnDnsSettingsNotificiationDialogView::OnClosing,
                     base::Unretained(this)));
  SetAcceptCallback(
      base::BindOnce(&BraveVpnDnsSettingsNotificiationDialogView::OnAccept,
                     base::Unretained(this)));
  auto* header_label =
      AddChildView(std::make_unique<views::Label>(l10n_util::GetStringUTF16(
          IDS_BRAVE_VPN_DNS_SETTINGS_NOTIFICATION_DIALOG_TITLE)));
  header_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  const std::u16string contents_text = l10n_util::GetStringUTF16(
      IDS_BRAVE_VPN_DNS_SETTINGS_NOTIFICATION_DIALOG_TEXT);

  std::u16string learn_more_link_text = l10n_util::GetStringUTF16(
      IDS_BRAVE_VPN_DNS_SETTINGS_NOTIFICATION_DIALOG_LEARN_MORE_TEXT);
  std::u16string full_text = l10n_util::GetStringFUTF16(
      IDS_BRAVE_VPN_DNS_SETTINGS_NOTIFICATION_DIALOG_TEXT,
      learn_more_link_text);
  const int main_message_length =
      full_text.size() - learn_more_link_text.size();

  auto* contents_label = AddChildView(std::make_unique<views::StyledLabel>());
  contents_label->SetTextContext(views::style::CONTEXT_DIALOG_BODY_TEXT);
  views::StyledLabel::RangeStyleInfo message_style;
  contents_label->SetText(full_text);
  contents_label->AddStyleRange(gfx::Range(0, main_message_length),
                                message_style);
  contents_label->SizeToFit(kDialogWidth);

  // Add "Learn more" link.
  views::StyledLabel::RangeStyleInfo link_style =
      views::StyledLabel::RangeStyleInfo::CreateForLink(base::BindRepeating(
          &BraveVpnDnsSettingsNotificiationDialogView::OnLearnMoreLinkClicked,
          base::Unretained(this)));
  contents_label->AddStyleRange(
      gfx::Range(main_message_length, full_text.size()), link_style);
  contents_label->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT);

  dont_ask_again_checkbox_ =
      AddChildView(std::make_unique<views::Checkbox>(l10n_util::GetStringUTF16(
          IDS_BRAVE_VPN_DNS_SETTINGS_NOTIFICATION_DIALOG_CHECKBOX_TEXT)));
}

BraveVpnDnsSettingsNotificiationDialogView::
    ~BraveVpnDnsSettingsNotificiationDialogView() = default;

void BraveVpnDnsSettingsNotificiationDialogView::OnLearnMoreLinkClicked() {
  chrome::AddSelectedTabWithURL(browser_, GURL(kBraveVPNLearnMoreURL),
                                ui::PAGE_TRANSITION_AUTO_TOPLEVEL);
  AcceptDialog();
}

ui::mojom::ModalType BraveVpnDnsSettingsNotificiationDialogView::GetModalType()
    const {
  return ui::mojom::ModalType::kWindow;
}

bool BraveVpnDnsSettingsNotificiationDialogView::ShouldShowCloseButton() const {
  return false;
}

bool BraveVpnDnsSettingsNotificiationDialogView::ShouldShowWindowTitle() const {
  return false;
}

void BraveVpnDnsSettingsNotificiationDialogView::OnAccept() {
  close_window_ = true;
}

void BraveVpnDnsSettingsNotificiationDialogView::OnClosing() {
  prefs_->SetBoolean(prefs::kBraveVPNShowNotificationDialog,
                     !dont_ask_again_checkbox_->GetChecked());
}

BEGIN_METADATA(BraveVpnDnsSettingsNotificiationDialogView)
END_METADATA

}  // namespace brave_vpn
