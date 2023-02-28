// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_COMMANDS_BROWSER_ACCELERATOR_PREF_MANAGER_H_
#define BRAVE_COMPONENTS_COMMANDS_BROWSER_ACCELERATOR_PREF_MANAGER_H_

#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/containers/flat_map.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace commands {

class COMPONENT_EXPORT(COMMANDS_BROWSER) AcceleratorPrefManager {
 public:
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  explicit AcceleratorPrefManager(PrefService* prefs);
  AcceleratorPrefManager(const AcceleratorPrefManager&) = delete;
  AcceleratorPrefManager& operator=(const AcceleratorPrefManager&) = delete;
  ~AcceleratorPrefManager();

  void ClearAccelerators(int command_id);
  void AddAccelerator(int command_id, const std::string& accelerator);
  void RemoveAccelerator(int command_id, const std::string& accelerator);

  base::flat_map<int, std::vector<std::string>> GetAccelerators();

 private:
  raw_ptr<PrefService> prefs_;
};

}  // namespace commands

#endif  // BRAVE_COMPONENTS_COMMANDS_BROWSER_ACCELERATOR_PREF_MANAGER_H_
