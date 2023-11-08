/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/brave_global_confirm_infobar.h"
#include <memory>
#include <utility>
#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"

std::unique_ptr<infobars::InfoBar> CreateBraveGlobalConfirmInfoBar(
    std::unique_ptr<BraveConfirmInfoBarDelegate> delegate) {
  return std::make_unique<BraveConfirmInfoBar>(std::move(delegate));
}

class BraveGlobalConfirmInfoBar::DelegateProxy
    : public BraveConfirmInfoBarDelegate {
 public:
  explicit DelegateProxy(
      base::WeakPtr<BraveGlobalConfirmInfoBar> global_info_bar);

  DelegateProxy(const DelegateProxy&) = delete;
  DelegateProxy& operator=(const DelegateProxy&) = delete;

  ~DelegateProxy() override;
  void Detach();

 private:
  friend class BraveGlobalConfirmInfoBar;

  // BraveConfirmInfoBarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  std::u16string GetLinkText() const override;
  GURL GetLinkURL() const override;
  void InfoBarDismissed() override;
  std::u16string GetMessageText() const override;
  gfx::ElideBehavior GetMessageElideBehavior() const override;
  std::vector<int> GetButtonsOrder() const override;
  int GetButtons() const override;
  std::u16string GetButtonLabel(InfoBarButton button) const override;
  bool Accept() override;
  bool Cancel() override;
  bool IsCloseable() const override;
  bool ShouldAnimate() const override;
  bool InfobarAction(
      base::OnceCallback<void(base::WeakPtr<BraveGlobalConfirmInfoBar>)>
          action);

  base::WeakPtr<BraveGlobalConfirmInfoBar> global_info_bar_;
};

BraveGlobalConfirmInfoBar::DelegateProxy::DelegateProxy(
    base::WeakPtr<BraveGlobalConfirmInfoBar> global_info_bar)
    : global_info_bar_(global_info_bar) {}

BraveGlobalConfirmInfoBar::DelegateProxy::~DelegateProxy() = default;

infobars::InfoBarDelegate::InfoBarIdentifier
BraveGlobalConfirmInfoBar::DelegateProxy::GetIdentifier() const {
  return global_info_bar_ ? global_info_bar_->delegate_->GetIdentifier()
                          : INVALID;
}

std::u16string BraveGlobalConfirmInfoBar::DelegateProxy::GetLinkText() const {
  return global_info_bar_ ? global_info_bar_->delegate_->GetLinkText()
                          : BraveConfirmInfoBarDelegate::GetLinkText();
}

GURL BraveGlobalConfirmInfoBar::DelegateProxy::GetLinkURL() const {
  return global_info_bar_ ? global_info_bar_->delegate_->GetLinkURL()
                          : BraveConfirmInfoBarDelegate::GetLinkURL();
}

bool BraveGlobalConfirmInfoBar::DelegateProxy::IsCloseable() const {
  return global_info_bar_ ? global_info_bar_->delegate_->IsCloseable()
                          : BraveConfirmInfoBarDelegate::IsCloseable();
}

bool BraveGlobalConfirmInfoBar::DelegateProxy::ShouldAnimate() const {
  return global_info_bar_ ? global_info_bar_->delegate_->ShouldAnimate()
                          : BraveConfirmInfoBarDelegate::ShouldAnimate();
}

bool BraveGlobalConfirmInfoBar::DelegateProxy::InfobarAction(
    base::OnceCallback<void(base::WeakPtr<BraveGlobalConfirmInfoBar>)> action) {
  if (global_info_bar_) {
    global_info_bar_->OnInfoBarRemoved(infobar(), false);
    if (action) {
      std::move(action).Run(global_info_bar_);
    }
    if (global_info_bar_) {
      global_info_bar_->Close();
    }
    return true;
  }
  return false;
}

void BraveGlobalConfirmInfoBar::DelegateProxy::InfoBarDismissed() {
  if (!InfobarAction(base::BindOnce(
          [](base::WeakPtr<BraveGlobalConfirmInfoBar> global_info_bar) {
            global_info_bar->delegate_->InfoBarDismissed();
          }))) {
    BraveConfirmInfoBarDelegate::InfoBarDismissed();
  }
}

std::u16string BraveGlobalConfirmInfoBar::DelegateProxy::GetMessageText()
    const {
  return global_info_bar_ ? global_info_bar_->delegate_->GetMessageText()
                          : std::u16string();
}

gfx::ElideBehavior
BraveGlobalConfirmInfoBar::DelegateProxy::GetMessageElideBehavior() const {
  return global_info_bar_
             ? global_info_bar_->delegate_->GetMessageElideBehavior()
             : BraveConfirmInfoBarDelegate::GetMessageElideBehavior();
}

int BraveGlobalConfirmInfoBar::DelegateProxy::GetButtons() const {
  return global_info_bar_ ? global_info_bar_->delegate_->GetButtons()
                          : BUTTON_NONE;
}

std::u16string BraveGlobalConfirmInfoBar::DelegateProxy::GetButtonLabel(
    InfoBarButton button) const {
  return global_info_bar_ ? global_info_bar_->delegate_->GetButtonLabel(button)
                          : BraveConfirmInfoBarDelegate::GetButtonLabel(button);
}

std::vector<int> BraveGlobalConfirmInfoBar::DelegateProxy::GetButtonsOrder()
    const {
  return global_info_bar_ ? global_info_bar_->delegate_->GetButtonsOrder()
                          : BraveConfirmInfoBarDelegate::GetButtonsOrder();
}

bool BraveGlobalConfirmInfoBar::DelegateProxy::Accept() {
  if (InfobarAction(base::BindOnce(
          [](base::WeakPtr<BraveGlobalConfirmInfoBar> global_info_bar) {
            global_info_bar->delegate_->Accept();
          }))) {
    return true;
  }
  return BraveConfirmInfoBarDelegate::Accept();
}

bool BraveGlobalConfirmInfoBar::DelegateProxy::Cancel() {
  if (InfobarAction(base::BindOnce(
          [](base::WeakPtr<BraveGlobalConfirmInfoBar> global_info_bar) {
            global_info_bar->delegate_->Cancel();
          }))) {
    return true;
  }
  return BraveConfirmInfoBarDelegate::Cancel();
}

void BraveGlobalConfirmInfoBar::DelegateProxy::Detach() {
  global_info_bar_.reset();
}

BraveGlobalConfirmInfoBar::BraveGlobalConfirmInfoBar(
    std::unique_ptr<BraveConfirmInfoBarDelegate> delegate)
    : delegate_(std::move(delegate)) {
  browser_tab_strip_tracker_.Init();
}
BraveGlobalConfirmInfoBar::~BraveGlobalConfirmInfoBar() {
  while (!proxies_.empty()) {
    auto it = proxies_.begin();
    it->second->Detach();
    it->first->RemoveObserver(this);
    it->first->RemoveInfoBar(it->second->infobar());
    proxies_.erase(it);
  }
}

BraveGlobalConfirmInfoBar* BraveGlobalConfirmInfoBar::Show(
    std::unique_ptr<BraveConfirmInfoBarDelegate> delegate) {
  return new BraveGlobalConfirmInfoBar(std::move(delegate));
}

void BraveGlobalConfirmInfoBar::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (!selection.new_contents) {
    return;
  }

  MaybeAddInfoBar(selection.new_contents);

  if (!selection.old_contents) {
    return;
  }

  infobars::ContentInfoBarManager* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(selection.old_contents);

  if (!selection.selection_changed() || !infobar_manager ||
      !base::Contains(proxies_, infobar_manager)) {
    return;
  }

  proxies_[infobar_manager]->Detach();
  infobar_manager->RemoveObserver(this);
  infobar_manager->RemoveInfoBar(proxies_[infobar_manager]->infobar());
  proxies_.erase(infobar_manager);
}

void BraveGlobalConfirmInfoBar::MaybeAddInfoBar(
    content::WebContents* web_contents) {
  infobars::ContentInfoBarManager* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(web_contents);
  DCHECK(infobar_manager);
  if (base::Contains(proxies_, infobar_manager)) {
    return;
  }

  auto proxy = std::make_unique<BraveGlobalConfirmInfoBar::DelegateProxy>(
      weak_factory_.GetWeakPtr());
  BraveGlobalConfirmInfoBar::DelegateProxy* proxy_ptr = proxy.get();
  if (infobars::InfoBar const* added_bar = infobar_manager->AddInfoBar(
          CreateBraveGlobalConfirmInfoBar(std::move(proxy)));
      !added_bar) {
    return;
  }

  infobar_manager->AddObserver(this);
  proxies_[infobar_manager] = proxy_ptr;
}

void BraveGlobalConfirmInfoBar::OnInfoBarRemoved(infobars::InfoBar* info_bar,
                                                 bool animate) {
  OnManagerShuttingDown(info_bar->owner());
}

void BraveGlobalConfirmInfoBar::OnManagerShuttingDown(
    infobars::InfoBarManager* manager) {
  manager->RemoveObserver(this);
  proxies_.erase(manager);
}

void BraveGlobalConfirmInfoBar::Close() {
  delete this;
}
