/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_vpn/brave_vpn_fallback_dialog_view.h"

#include <memory>
#include <utility>

#include "brave/browser/brave_features_internal_names.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "chrome/browser/about_flags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/flags_ui/pref_service_flags_storage.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/link.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/box_layout.h"

namespace brave {
void ShowBraveVpnIKEv2FallbackDialog() {
  brave_vpn::BraveVpnFallbackDialogView::Show(chrome::FindLastActive());
}
}  // namespace brave

namespace brave_vpn {

namespace {

constexpr char kBraveVPNLearnMoreURL[] =
    "https://support.brave.com/hc/en-us/articles/";

constexpr int kChildSpacing = 16;
constexpr int kPadding = 24;
constexpr int kTopPadding = 32;
constexpr int kBottomPadding = 26;
constexpr int kDialogWidth = 600;

}  // namespace

// static
void BraveVpnFallbackDialogView::Show(Browser* browser) {
  auto* prefs = browser->profile()->GetPrefs();
  if (!prefs->GetBoolean(prefs::kBraveVPNWireguardFallbackDialog)) {
    return;
  }

  constrained_window::CreateBrowserModalDialogViews(
      new BraveVpnFallbackDialogView(browser),
      browser->window()->GetNativeWindow())
      ->Show();
}

BraveVpnFallbackDialogView::BraveVpnFallbackDialogView(Browser* browser)
    : browser_(browser), prefs_(browser->profile()->GetPrefs()) {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::TLBR(kTopPadding, kPadding, kBottomPadding, kPadding),
      kChildSpacing));
  SetButtons(ui::DIALOG_BUTTON_OK | ui::DIALOG_BUTTON_CANCEL);
  SetButtonLabel(
      ui::DIALOG_BUTTON_OK,
      l10n_util::GetStringUTF16(IDS_BRAVE_VPN_FALLBACK_DIALOG_OK_TEXT));
  SetButtonLabel(
      ui::DIALOG_BUTTON_CANCEL,
      l10n_util::GetStringUTF16(IDS_BRAVE_VPN_FALLBACK_DIALOG_CANCEL_TEXT));
  SetAcceptCallback(base::BindOnce(&BraveVpnFallbackDialogView::OnAccept,
                                   base::Unretained(this)));
  auto* header_label = AddChildView(std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_BRAVE_VPN_FALLBACK_DIALOG_TITLE)));
  header_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  const std::u16string contents_text =
      l10n_util::GetStringUTF16(IDS_BRAVE_VPN_FALLBACK_DIALOG_TEXT);

  std::u16string learn_more_link_text =
      l10n_util::GetStringUTF16(IDS_BRAVE_VPN_FALLBACK_DIALOG_LEARN_MORE_TEXT);
  std::u16string full_text = l10n_util::GetStringFUTF16(
      IDS_BRAVE_VPN_FALLBACK_DIALOG_TEXT, learn_more_link_text);
  const int main_message_length =
      full_text.size() - learn_more_link_text.size();

  auto* contents_label = AddChildView(std::make_unique<views::StyledLabel>());
  contents_label->SetTextContext(views::style::CONTEXT_DIALOG_BODY_TEXT);
  views::StyledLabel::RangeStyleInfo message_style;
  contents_label->SetText(full_text);
  contents_label->AddStyleRange(gfx::Range(0, main_message_length),
                                message_style);
  contents_label->SizeToFit(kDialogWidth);

  RegisterWindowClosingCallback(base::BindOnce(
      &BraveVpnFallbackDialogView::OnClosing, base::Unretained(this)));

  // Add "Learn more" link.
  views::StyledLabel::RangeStyleInfo link_style =
      views::StyledLabel::RangeStyleInfo::CreateForLink(base::BindRepeating(
          &BraveVpnFallbackDialogView::OnLearnMoreLinkClicked,
          base::Unretained(this)));
  contents_label->AddStyleRange(
      gfx::Range(main_message_length, full_text.size()), link_style);
  contents_label->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_LEFT);

  dont_ask_again_checkbox_ =
      AddChildView(std::make_unique<views::Checkbox>(l10n_util::GetStringUTF16(
          IDS_BRAVE_VPN_DNS_SETTINGS_NOTIFICATION_DIALOG_CHECKBOX_TEXT)));
}

BraveVpnFallbackDialogView::~BraveVpnFallbackDialogView() = default;

void BraveVpnFallbackDialogView::OnLearnMoreLinkClicked() {
  chrome::AddSelectedTabWithURL(browser_, GURL(kBraveVPNLearnMoreURL),
                                ui::PAGE_TRANSITION_AUTO_TOPLEVEL);
  CancelDialog();
}

ui::ModalType BraveVpnFallbackDialogView::GetModalType() const {
  return ui::MODAL_TYPE_WINDOW;
}

bool BraveVpnFallbackDialogView::ShouldShowCloseButton() const {
  return false;
}

bool BraveVpnFallbackDialogView::ShouldShowWindowTitle() const {
  return false;
}

void BraveVpnFallbackDialogView::OnClosing() {
  prefs_->SetBoolean(prefs::kBraveVPNWireguardFallbackDialog,
                     !dont_ask_again_checkbox_->GetChecked());
}

void BraveVpnFallbackDialogView::OnAccept() {
  g_browser_process->local_state()->SetBoolean(prefs::kBraveVPNWireguardEnabled,
                                               false);
  chrome::AttemptRestart();
}

BEGIN_METADATA(BraveVpnFallbackDialogView, views::DialogDelegateView)
END_METADATA

}  // namespace brave_vpn
