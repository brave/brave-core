/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_INTERACTIVE_STATUS_ICON_MENU_MODEL_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_INTERACTIVE_STATUS_ICON_MENU_MODEL_H_

#include <map>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "ui/base/models/image_model.h"
#include "ui/base/models/simple_menu_model.h"

namespace gfx {
class Image;
}

class BraveVpnMenuModel : public ui::SimpleMenuModel,
                          public ui::SimpleMenuModel::Delegate,
                          public base::SupportsWeakPtr<BraveVpnMenuModel> {
 public:
  class Delegate {
   public:
    // Performs the action associates with the specified command id.
    // The passed |event_flags| are the flags from the event which issued this
    // command and they can be examined to find modifier keys.
    virtual void ExecuteCommand(int command_id, int event_flags) = 0;

   protected:
    virtual ~Delegate() {}
  };

  // The Delegate can be NULL.
  explicit BraveVpnMenuModel(Delegate* delegate);

  BraveVpnMenuModel(const BraveVpnMenuModel&) = delete;
  BraveVpnMenuModel& operator=(const BraveVpnMenuModel&) = delete;

  ~BraveVpnMenuModel() override;
  std::u16string GetLabelForCommandId(int command_id) const override;
  ui::ImageModel GetIconForCommandId(int command_id) const override;
  Delegate* delegate() { return delegate_; }

  void RebuildMenu();

 protected:
  void set_delegate(Delegate* delegate) { delegate_ = delegate; }

 private:
  // Overridden from ui::SimpleMenuModel::Delegate:
  void ExecuteCommand(int command_id, int event_flags) override;

  struct ItemState;

  // Map the properties to the command id (used as key).
  typedef std::map<int, ItemState> ItemStateMap;

  ItemStateMap item_states_;

  raw_ptr<Delegate> delegate_;
};

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_WIN_BRAVE_VPN_WIREGUARD_SERVICE_INTERACTIVE_STATUS_ICON_MENU_MODEL_H_
