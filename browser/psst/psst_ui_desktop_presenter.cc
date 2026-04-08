// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_ui_desktop_presenter.h"

#include "brave/browser/psst/psst_infobar_delegate.h"
#include "brave/components/psst/common/constants.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "content/public/browser/web_contents.h"

namespace {

constexpr int kDialogMinHeight = 100;
constexpr int kDialogMaxHeight = 700;
constexpr int kDialogWidth = 475;
const char kUiDesktopDelegateUserDataKey[] = "UiDesktopDelegateUserData";

class UiDesktopDelegateUserData : public base::SupportsUserData::Data {
 public:
  explicit UiDesktopDelegateUserData(
      psst::PsstUiDesktopPresenter::PsstUiDesktopDelegate* delegate)
      : delegate_(delegate) {}
  ~UiDesktopDelegateUserData() override = default;
  UiDesktopDelegateUserData(const UiDesktopDelegateUserData&) = delete;
  UiDesktopDelegateUserData& operator=(const UiDesktopDelegateUserData&) =
      delete;

  psst::PsstUiDesktopPresenter::PsstUiDesktopDelegate* delegate() {
    return delegate_;
  }

 private:
  raw_ptr<psst::PsstUiDesktopPresenter::PsstUiDesktopDelegate>
      delegate_;  // unowned
};

void OpenPsstDialog(content::WebContents* initiator) {
  const gfx::Size min_size(kDialogWidth, kDialogMinHeight);
  const gfx::Size max_size(kDialogWidth, kDialogMaxHeight);
  auto ui_desktop_delegate =
      std::make_unique<psst::PsstUiDesktopPresenter::PsstUiDesktopDelegate>(
          initiator->GetWeakPtr());
  auto* ui_desktop_delegate_ptr = ui_desktop_delegate.get();
  auto* delegate = ShowConstrainedWebDialogWithAutoResize(
      initiator->GetBrowserContext(), std::move(ui_desktop_delegate), initiator,
      min_size, max_size);
  CHECK(delegate);
  CHECK(ui_desktop_delegate_ptr);

  ui_desktop_delegate_ptr->SetDelegateToWebContents(
      delegate->GetWebContents()->GetWeakPtr());
}
}  // namespace

namespace psst {

// static
PsstUiDesktopPresenter::PsstUiDesktopDelegate*
PsstUiDesktopPresenter::PsstUiDesktopDelegate::GetDelegateFromWebContents(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return nullptr;
  }
  auto* user_data = static_cast<UiDesktopDelegateUserData*>(
      web_contents->GetUserData(kUiDesktopDelegateUserDataKey));
  return user_data ? user_data->delegate() : nullptr;
}

PsstUiDesktopPresenter::PsstUiDesktopDelegate::PsstUiDesktopDelegate(
    base::WeakPtr<content::WebContents> initiator_web_contents)
    : initiator_web_contents_(std::move(initiator_web_contents)) {
  set_dialog_content_url(GURL(psst::kBraveUIPsstURL));
  set_show_dialog_title(false);
  set_can_close(true);
}
PsstUiDesktopPresenter::PsstUiDesktopDelegate::~PsstUiDesktopDelegate() =
    default;

void PsstUiDesktopPresenter::PsstUiDesktopDelegate::SetDelegateToWebContents(
    base::WeakPtr<content::WebContents> dialog_web_contents) {
  if (!dialog_web_contents) {
    return;
  }
  dialog_web_contents->SetUserData(
      kUiDesktopDelegateUserDataKey,
      std::make_unique<UiDesktopDelegateUserData>(this));
  dialog_web_contents_ = std::move(dialog_web_contents);
}

content::WebContents*
PsstUiDesktopPresenter::PsstUiDesktopDelegate::GetInitiatorWebContents() const {
  return initiator_web_contents_.get();
}

void PsstUiDesktopPresenter::PsstUiDesktopDelegate::OnDialogClosed(
    const std::string& /* json_retval */) {
  if (!dialog_web_contents_) {
    return;
  }
  dialog_web_contents_->RemoveUserData(kUiDesktopDelegateUserDataKey);
  dialog_web_contents_ = nullptr;
  initiator_web_contents_ = nullptr;
}

PsstUiDesktopPresenter::PsstUiDesktopPresenter(
    base::WeakPtr<content::WebContents> web_contents)
    : web_contents_(std::move(web_contents)) {}
PsstUiDesktopPresenter::~PsstUiDesktopPresenter() = default;

void PsstUiDesktopPresenter::ShowInfoBar(InfoBarCallback on_accept_callback) {
  if (!web_contents_) {
    return;
  }

  infobars::ContentInfoBarManager* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(web_contents_.get());
  if (!infobar_manager) {
    return;
  }

  PsstInfoBarDelegate::Create(infobar_manager, std::move(on_accept_callback));
}

void PsstUiDesktopPresenter::ShowConsentDialog() {
  if (!web_contents_) {
    return;
  }

  OpenPsstDialog(web_contents_.get());
}

}  // namespace psst
