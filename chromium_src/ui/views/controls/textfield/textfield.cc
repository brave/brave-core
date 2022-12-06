// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/views/controls/textfield/textfield.h"
#include "base/logging.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield_controller.h"

namespace views {

bool Textfield::AcceleratorPressed(const ui::Accelerator& accelerator) {
  ui::KeyEvent event(
      accelerator.key_state() == ui::Accelerator::KeyState::PRESSED
          ? ui::ET_KEY_PRESSED
          : ui::ET_KEY_RELEASED,
      accelerator.key_code(), accelerator.modifiers());
  auto command = GetCommandForKeyEvent(event);
  if ((text_input_type_ != ui::TEXT_INPUT_TYPE_PASSWORD) &&
      (command != ui::TextEditCommand::COPY || !controller_ ||
       !controller_->SelectedTextIsURL())) {
    return Textfield::AcceleratorPressed_ChromiumImpl(accelerator);
  }
  controller_->OnSanitizedCopy(ui::ClipboardBuffer::kCopyPaste);
  return true;
}

}  // namespace views

#define GET_ACCELERATOR_FOR_COMMAND_ID                                         \
  if (controller_ && controller_->SelectedTextIsURL()) {                       \
    if (command_id == kCopy) {                                                 \
      return false;                                                            \
    }                                                                          \
    if (controller_->IsCleanLinkCommand(command_id)) {                         \
      *accelerator = ui::Accelerator(ui::VKEY_C, ui::EF_PLATFORM_ACCELERATOR); \
      return true;                                                             \
    }                                                                          \
  }

#define AcceleratorPressed AcceleratorPressed_ChromiumImpl
#include "src/ui/views/controls/textfield/textfield.cc"
#undef AcceleratorPressed
#undef GET_ACCELERATOR_FOR_COMMAND_ID
