/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_ICON_TRAY_MENU_MODEL_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_ICON_TRAY_MENU_MODEL_H_

#include "base/memory/raw_ptr.h"
#include "ui/menus/simple_menu_model.h"

namespace brave_vpn {

class TrayMenuModel : public ui::SimpleMenuModel,
                      public ui::SimpleMenuModel::Delegate {
 public:
  class Delegate {
   public:
    virtual void ExecuteCommand(int command_id, int event_flags) = 0;
    virtual void OnMenuWillShow(ui::SimpleMenuModel* source) = 0;

   protected:
    virtual ~Delegate() {}
  };

  // The Delegate can be NULL.
  explicit TrayMenuModel(Delegate* delegate);

  TrayMenuModel(const TrayMenuModel&) = delete;
  TrayMenuModel& operator=(const TrayMenuModel&) = delete;

  ~TrayMenuModel() override;
  Delegate* delegate() { return delegate_; }

  // Overridden from ui::SimpleMenuModel::Delegate:
  void ExecuteCommand(int command_id, int event_flags) override;
  void OnMenuWillShow(ui::SimpleMenuModel* source) override;

 protected:
  void set_delegate(Delegate* delegate) { delegate_ = delegate; }

 private:
  raw_ptr<Delegate> delegate_;
};

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_STATUS_TRAY_STATUS_ICON_TRAY_MENU_MODEL_H_
