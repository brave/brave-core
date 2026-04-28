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

base::WeakPtr<psst::PsstUiDesktopPresenter::PsstUiDesktopDelegate>
OpenPsstDialog(content::WebContents* initiator) {
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

  ui_desktop_delegate_ptr->SetDelegateToWebContents(delegate);
  return ui_desktop_delegate_ptr->GetWeakPtr();
}

content::WebContents* GetDialogWebContents(
    ConstrainedWebDialogDelegate* dialog_delegate) {
  if (!dialog_delegate) {
    return nullptr;
  }
  return dialog_delegate->GetWebContents();
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
    ConstrainedWebDialogDelegate* dialog_delegate) {
  auto* dialog_web_contents = GetDialogWebContents(dialog_delegate);
  if (!dialog_web_contents) {
    return;
  }
  web_dialog_delegate_ = dialog_delegate;

  dialog_web_contents->SetUserData(
      kUiDesktopDelegateUserDataKey,
      std::make_unique<UiDesktopDelegateUserData>(this));
}

content::WebContents*
PsstUiDesktopPresenter::PsstUiDesktopDelegate::GetInitiatorWebContents() const {
  return initiator_web_contents_.get();
}

void PsstUiDesktopPresenter::PsstUiDesktopDelegate::OnDialogClosed(
    const std::string& /* json_retval */) {
  auto* dialog_web_contents = GetDialogWebContents(web_dialog_delegate_);
  if (!dialog_web_contents) {
    return;
  }
  dialog_web_contents->RemoveUserData(kUiDesktopDelegateUserDataKey);
  initiator_web_contents_ = nullptr;
  web_dialog_delegate_ = nullptr;
}

base::WeakPtr<PsstUiDesktopPresenter::PsstUiDesktopDelegate>
PsstUiDesktopPresenter::PsstUiDesktopDelegate::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

bool PsstUiDesktopPresenter::PsstUiDesktopDelegate::IsDialogShown() const {
  return GetDialogWebContents(web_dialog_delegate_) != nullptr;
}

void PsstUiDesktopPresenter::PsstUiDesktopDelegate::CloseDialog() {
  if (!web_dialog_delegate_) {
    return;
  }

  web_dialog_delegate_->OnDialogCloseFromWebUI();
  web_dialog_delegate_->GetWebDialogDelegate()->OnDialogClosed({});
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

  dialog_delegate_ = OpenPsstDialog(web_contents_.get());
}

bool PsstUiDesktopPresenter::IsDialogShown() const {
  if (!dialog_delegate_) {
    return false;
  }

  return dialog_delegate_->IsDialogShown();
}

}  // namespace psst
