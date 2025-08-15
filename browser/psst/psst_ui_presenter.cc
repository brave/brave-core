
// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_ui_presenter.h"

#include "brave/browser/psst/brave_psst_infobar_delegate.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

namespace psst {

namespace {

constexpr int kDialogMinHeight = 100;
constexpr int kDialogMaxHeight = 700;
constexpr int kDialogWidth = 475;

class PsstWebDialogDelegate : public ui::WebDialogDelegate {
 public:
  PsstWebDialogDelegate();
  PsstWebDialogDelegate(const PsstWebDialogDelegate&) = delete;
  PsstWebDialogDelegate& operator=(const PsstWebDialogDelegate&) = delete;
  ~PsstWebDialogDelegate() override;

  GURL GetDialogContentURL() const override;
  void OnDialogClosed(const std::string& json_retval) override;
  void OnCloseContents(content::WebContents* source,
                       bool* out_close_dialog) override;
};

PsstWebDialogDelegate::PsstWebDialogDelegate() {
  set_show_dialog_title(false);
}

PsstWebDialogDelegate::~PsstWebDialogDelegate() = default;

GURL PsstWebDialogDelegate::GetDialogContentURL() const {
  return GURL(kBraveUIPsstURL);
}

void PsstWebDialogDelegate::OnDialogClosed(
    const std::string& /* json_retval */) {}

void PsstWebDialogDelegate::OnCloseContents(content::WebContents* /* source */,
                                            bool* out_close_dialog) {
  *out_close_dialog = true;
}

void OpenPsstDialog(content::WebContents* initiator) {
  const gfx::Size min_size(kDialogWidth, kDialogMinHeight);
  const gfx::Size max_size(kDialogWidth, kDialogMaxHeight);
  ShowConstrainedWebDialogWithAutoResize(
      initiator->GetBrowserContext(), std::make_unique<PsstWebDialogDelegate>(),
      initiator, min_size, max_size);
}

}  // namespace

UiDesktopPresenter::UiDesktopPresenter(content::WebContents* web_contents)
    : web_contents_(web_contents) {}
UiDesktopPresenter::~UiDesktopPresenter() = default;

void UiDesktopPresenter::ShowInfoBar(
    BravePsstInfoBarDelegate::AcceptCallback on_accept_callback) {
  infobars::ContentInfoBarManager* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(web_contents_);
  if (!infobar_manager) {
    return;
  }

  BravePsstInfoBarDelegate::Create(
      infobar_manager, base::BindOnce(&UiDesktopPresenter::OnInfobarAccepted,
                                      weak_ptr_factory_.GetWeakPtr(),
                                      std::move(on_accept_callback)));
}

void UiDesktopPresenter::ShowIcon() {
  // Implementation to show the icon in the UI.
  // This could involve updating the UI state or notifying the user.
}

void UiDesktopPresenter::OnInfobarAccepted(
    BravePsstInfoBarDelegate::AcceptCallback on_accept_callback,
    const bool is_accepted) {
  if (on_accept_callback) {
    std::move(on_accept_callback).Run(is_accepted);
  }
  // Open the Psst dialog when the infobar is accepted.
  OpenPsstDialog(web_contents_);
}

}  // namespace psst
