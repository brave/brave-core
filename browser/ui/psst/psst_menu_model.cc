// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/psst/psst_menu_model.h"

#include "brave/app/brave_command_ids.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace psst {

PsstMenuModel::PsstMenuModel() : ui::SimpleMenuModel(this) {
  BuildMenuItems();
}

PsstMenuModel::~PsstMenuModel() = default;

void PsstMenuModel::AddObserver(Observer* obs) {
  observer_list_.AddObserver(obs);
}

void PsstMenuModel::RemoveObserver(Observer* obs) {
  observer_list_.RemoveObserver(obs);
}

void PsstMenuModel::ExecuteCommand(int command_id, int event_flags) {
  for (Observer& obs : observer_list_) {
    if (command_id == IDC_PSST_DONT_SHOW_FOR_THIS_SITE) {
      obs.OnDontShowThisSiteSelected();
    } else if (command_id == IDC_PSST_DISABLE_PRIVACY_SETTINGS_TUNING) {
      obs.OnDisablePrivacySettingsTuningSelected();
    }
  }
}

bool PsstMenuModel::IsCommandIdEnabled(int command_id) const {
  return true;
}

void PsstMenuModel::BuildMenuItems() {
  AddItem(IDC_PSST_DONT_SHOW_FOR_THIS_SITE,
          l10n_util::GetStringUTF16(IDS_IDC_PSST_DONT_SHOW_FOR_THIS_SITE));
  AddItem(
      IDC_PSST_DISABLE_PRIVACY_SETTINGS_TUNING,
      l10n_util::GetStringUTF16(IDS_IDC_PSST_DISABLE_PRIVACY_SETTINGS_TUNING));
}

}  // namespace psst
