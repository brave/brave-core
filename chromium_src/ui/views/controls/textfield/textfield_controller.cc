// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/views/controls/textfield/textfield_controller.h"

namespace views {

bool TextfieldController::SelectedTextIsURL() {
  return false;
}

bool TextfieldController::IsCleanLinkCommand(int command_id) const {
  return false;
}
}  // namespace views

#include "src/ui/views/controls/textfield/textfield_controller.cc"
