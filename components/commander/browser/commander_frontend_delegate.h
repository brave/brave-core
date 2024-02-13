// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_COMMANDER_BROWSER_COMMANDER_FRONTEND_DELEGATE_H_
#define BRAVE_COMPONENTS_COMMANDER_BROWSER_COMMANDER_FRONTEND_DELEGATE_H_

#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/observer_list_types.h"
#include "brave/components/commander/browser/commander_item_model.h"

namespace commander {
class COMPONENT_EXPORT(COMMANDER_BROWSER) CommanderFrontendDelegate {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnCommanderUpdated() = 0;
  };

  virtual ~CommanderFrontendDelegate() {}

  virtual void Toggle() = 0;
  virtual void Hide() = 0;
  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;
  virtual void SelectCommand(uint32_t command_index,
                             uint32_t result_set_id) = 0;
  virtual void UpdateText(const std::u16string& text) = 0;
  virtual std::vector<CommandItemModel> GetItems() = 0;
  virtual int GetResultSetId() = 0;
  virtual const std::u16string& GetPrompt() = 0;
};
}  // namespace commander

#endif  // BRAVE_COMPONENTS_COMMANDER_BROWSER_COMMANDER_FRONTEND_DELEGATE_H_
