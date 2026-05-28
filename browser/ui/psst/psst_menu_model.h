// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_PSST_PSST_MENU_MODEL_H_
#define BRAVE_BROWSER_UI_PSST_PSST_MENU_MODEL_H_

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "ui/menus/simple_menu_model.h"

namespace psst {

class PsstMenuModel : public ui::SimpleMenuModel,
                      public ui::SimpleMenuModel::Delegate {
 public:
  class Observer : public base::CheckedObserver {
   public:
    ~Observer() override = default;

    virtual void OnDontShowThisSiteSelected() = 0;
    virtual void OnDisablePrivacySettingsTuningSelected() = 0;
  };
  PsstMenuModel();
  ~PsstMenuModel() override;

  void AddObserver(Observer* obs);
  void RemoveObserver(Observer* obs);

  // ui::SimpleMenuModel::Delegate:
  void ExecuteCommand(int command_id, int event_flags) override;
  bool IsCommandIdEnabled(int command_id) const override;

 private:
  void BuildMenuItems();

  base::ObserverList<Observer> observer_list_;
};

}  // namespace psst

#endif  // BRAVE_BROWSER_UI_PSST_PSST_MENU_MODEL_H_
