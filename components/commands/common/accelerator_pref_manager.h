// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_COMMANDS_COMMON_ACCELERATOR_PREF_MANAGER_H_
#define BRAVE_COMPONENTS_COMMANDS_COMMON_ACCELERATOR_PREF_MANAGER_H_

#include <vector>

#include "base/component_export.h"
#include "base/containers/flat_map.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "ui/base/accelerators/accelerator.h"

namespace commands {

class COMPONENT_EXPORT(COMMANDS_COMMON) AcceleratorPrefManager {
 public:
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  explicit AcceleratorPrefManager(PrefService* prefs);
  AcceleratorPrefManager(const AcceleratorPrefManager&) = delete;
  AcceleratorPrefManager& operator=(const AcceleratorPrefManager&) = delete;
  ~AcceleratorPrefManager();

  void ClearAccelerators(int command_id);
  void AddAccelerator(int command_id, const ui::Accelerator& accelerator);
  void RemoveAccelerator(int command_id, const ui::Accelerator& accelerator);

  base::flat_map<int, std::vector<ui::Accelerator>> GetAccelerators();

 private:
  raw_ptr<PrefService> prefs_;
};

}  // namespace commands

#endif  // BRAVE_COMPONENTS_COMMANDS_COMMON_ACCELERATOR_PREF_MANAGER_H_
