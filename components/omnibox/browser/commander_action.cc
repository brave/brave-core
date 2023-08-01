// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/omnibox/browser/commander_action.h"

namespace commander {

CommanderAction::CommanderAction(uint32_t command_index, uint32_t result_set_id)
    : OmniboxAction({}, GURL()),
      command_index_(command_index),
      result_set_id_(result_set_id) {}
CommanderAction::~CommanderAction() = default;

void CommanderAction::Execute(ExecutionContext& context) const {
  // If we've generated and executed a command, our delegate must exist.
  auto* delegate = context.client_->GetCommanderDelegate();
  CHECK(delegate);
  delegate->SelectCommand(command_index_, result_set_id_);
}

}  // namespace commander
