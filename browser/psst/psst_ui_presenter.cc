// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_ui_presenter.h"

#include "brave/browser/psst/psst_infobar_delegate.h"
#include "brave/components/psst/common/constants.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "components/infobars/content/content_infobar_manager.h"

namespace psst {

namespace {

constexpr int kDialogMinHeight = 100;
constexpr int kDialogMaxHeight = 700;
constexpr int kDialogWidth = 475;

const char kUiDesktopDelegateUserDataKey[] = "UiDesktopDelegateUserData";

class UiDesktopDelegateUserData : public base::SupportsUserData::Data {
 public:
  explicit UiDesktopDelegateUserData(
      UiDesktopPresenter::UiDesktopDelegate* delegate)
      : delegate_(delegate) {}
  ~UiDesktopDelegateUserData() override = default;
  UiDesktopDelegateUserData(const UiDesktopDelegateUserData&) = delete;
  UiDesktopDelegateUserData& operator=(const UiDesktopDelegateUserData&) =
      delete;

  UiDesktopPresenter::UiDesktopDelegate* delegate() { return delegate_; }

 private:
  raw_ptr<UiDesktopPresenter::UiDesktopDelegate> delegate_;  // unowned
};

}  // namespace

UiDesktopPresenter::UiDesktopDelegate::UiDesktopDelegate(
    base::WeakPtr<content::WebContents> initiator_web_contents)
    : initiator_web_contents_(initiator_web_contents) {
  set_dialog_content_url(GURL(kBraveUIPsstURL));
  set_show_dialog_title(false);
  set_can_close(true);
}

UiDesktopPresenter::UiDesktopDelegate::~UiDesktopDelegate() = default;

// static
UiDesktopPresenter::UiDesktopDelegate*
UiDesktopPresenter::UiDesktopDelegate::GetDelegateFromWebContents(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return nullptr;
  }
  auto* user_data = static_cast<UiDesktopDelegateUserData*>(
      web_contents->GetUserData(kUiDesktopDelegateUserDataKey));
  return user_data ? user_data->delegate() : nullptr;
}

void UiDesktopPresenter::UiDesktopDelegate::SetDelegateToWebContents(
    content::WebContents* dialog_web_contents) {
  if (!dialog_web_contents) {
    return;
  }
  dialog_web_contents->SetUserData(
      kUiDesktopDelegateUserDataKey,
      std::make_unique<UiDesktopDelegateUserData>(this));
  dialog_web_contents_ = dialog_web_contents;
}

void UiDesktopPresenter::UiDesktopDelegate::OnDialogClosed(
    const std::string& /* json_retval */) {
  if (!dialog_web_contents_) {
    return;
  }
  dialog_web_contents_->RemoveUserData(kUiDesktopDelegateUserDataKey);
  dialog_web_contents_ = nullptr;
}

content::WebContents*
UiDesktopPresenter::UiDesktopDelegate::GetInitiatorWebContents() const {
  return initiator_web_contents_.get();
}

void OpenPsstDialog(content::WebContents* initiator) {
  const gfx::Size min_size(kDialogWidth, kDialogMinHeight);
  const gfx::Size max_size(kDialogWidth, kDialogMaxHeight);
  auto ui_desktop_delegate =
      std::make_unique<UiDesktopPresenter::UiDesktopDelegate>(initiator->GetWeakPtr());
  auto* ui_desktop_delegate_ptr = ui_desktop_delegate.get();
  auto* delegate = ShowConstrainedWebDialogWithAutoResize(
      initiator->GetBrowserContext(), std::move(ui_desktop_delegate), initiator,
      min_size, max_size);
  CHECK(delegate);
  CHECK(ui_desktop_delegate_ptr);

  ui_desktop_delegate_ptr->SetDelegateToWebContents(delegate->GetWebContents());
}

UiDesktopPresenter::UiDesktopPresenter(content::WebContents* web_contents)
    : web_contents_(web_contents) {}
UiDesktopPresenter::~UiDesktopPresenter() = default;

void UiDesktopPresenter::ShowInfoBar(
    PsstInfoBarDelegate::AcceptCallback on_accept_callback) {
  infobars::ContentInfoBarManager* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(web_contents_);
  if (!infobar_manager) {
    return;
  }

  PsstInfoBarDelegate::Create(
      infobar_manager, base::BindOnce(&UiDesktopPresenter::OnInfobarAccepted,
                                      weak_ptr_factory_.GetWeakPtr(),
                                      std::move(on_accept_callback)));
}

void UiDesktopPresenter::ShowIcon() {
  // Implementation to show the icon in the UI.
  // This could involve updating the UI state or notifying the user.
}

void UiDesktopPresenter::OnInfobarAccepted(
    PsstInfoBarDelegate::AcceptCallback on_accept_callback,
    const bool is_accepted) {
  if (on_accept_callback) {
    std::move(on_accept_callback).Run(is_accepted);
  }
  // Open the Psst dialog when the infobar is accepted.
  OpenPsstDialog(web_contents_);
}

}  // namespace psst
