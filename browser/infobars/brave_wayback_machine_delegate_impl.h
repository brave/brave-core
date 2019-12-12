/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_INFOBARS_BRAVE_WAYBACK_MACHINE_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_INFOBARS_BRAVE_WAYBACK_MACHINE_DELEGATE_IMPL_H_

#include <memory>

#include "brave/components/brave_wayback_machine/brave_wayback_machine_delegate.h"

class Profile;

class BraveWaybackMachineDelegateImpl : public BraveWaybackMachineDelegate {
 public:
  explicit BraveWaybackMachineDelegateImpl(Profile* profile);
  ~BraveWaybackMachineDelegateImpl() override;

  BraveWaybackMachineDelegateImpl(
      const BraveWaybackMachineDelegateImpl&) = delete;
  BraveWaybackMachineDelegateImpl& operator=(
      const BraveWaybackMachineDelegateImpl&) = delete;

 private:
  // BraveWaybackMachineDelegate overrides:
  bool IsWaybackMachineEnabled() const override;

  std::unique_ptr<infobars::InfoBar> CreateInfoBar(
      std::unique_ptr<BraveWaybackMachineInfoBarDelegate> delegate,
      content::WebContents* contents) override;

  Profile* profile_;
};

#endif  // BRAVE_BROWSER_INFOBARS_BRAVE_WAYBACK_MACHINE_DELEGATE_IMPL_H_
