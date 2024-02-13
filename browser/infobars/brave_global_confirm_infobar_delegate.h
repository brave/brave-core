/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_INFOBARS_BRAVE_GLOBAL_CONFIRM_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_INFOBARS_BRAVE_GLOBAL_CONFIRM_INFOBAR_DELEGATE_H_

#include <memory>

#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"

class BraveGlobalConfirmInfobarDelegate : public BraveConfirmInfoBarDelegate {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnInfoBarClosed() = 0;
  };

  BraveGlobalConfirmInfobarDelegate(const BraveGlobalConfirmInfobarDelegate&) =
      delete;
  BraveGlobalConfirmInfobarDelegate& operator=(
      const BraveGlobalConfirmInfobarDelegate&) = delete;
  ~BraveGlobalConfirmInfobarDelegate() override;

  bool Accept() override;
  bool Cancel() override;
  void InfoBarDismissed() override;

  void AddObserver(Observer* obs);
  void RemoveObserver(Observer* obs);

 protected:
  BraveGlobalConfirmInfobarDelegate();

 private:
  base::ObserverList<Observer> observer_list_;
};

class BraveGlobalConfirmInfoBarDelegateFactory {
 public:
  BraveGlobalConfirmInfoBarDelegateFactory(
      const BraveGlobalConfirmInfoBarDelegateFactory&) = delete;
  BraveGlobalConfirmInfoBarDelegateFactory& operator=(
      const BraveGlobalConfirmInfoBarDelegateFactory&) = delete;
  virtual ~BraveGlobalConfirmInfoBarDelegateFactory() = default;

  virtual std::unique_ptr<BraveGlobalConfirmInfobarDelegate> Create() = 0;
  virtual infobars::InfoBarDelegate::InfoBarIdentifier GetInfoBarIdentifier()
      const = 0;

 protected:
  BraveGlobalConfirmInfoBarDelegateFactory() = default;
};

#endif  // BRAVE_BROWSER_INFOBARS_BRAVE_GLOBAL_CONFIRM_INFOBAR_DELEGATE_H_
