// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_COMMANDER_ACTION_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_COMMANDER_ACTION_H_

#include "components/omnibox/browser/actions/omnibox_action.h"

namespace commander {

class CommanderAction : public OmniboxAction {
 public:
  CommanderAction(uint32_t command_index, uint32_t result_set_id);
  CommanderAction(const CommanderAction&) = delete;
  CommanderAction& operator=(const CommanderAction&) = delete;

  void Execute(ExecutionContext& context) const override;

 protected:
  ~CommanderAction() override;

 private:
  uint32_t command_index_;
  uint32_t result_set_id_;
};

}  // namespace commander

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_COMMANDER_ACTION_H_
