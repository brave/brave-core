// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_TEXTFIELD_TEXTFIELD_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_TEXTFIELD_TEXTFIELD_CONTROLLER_H_

#include "ui/base/clipboard/clipboard_buffer.h"

#define OnAfterCutOrCopy                                    \
  OnAfterCutOrCopy(ui::ClipboardBuffer clipboard_buffer) {} \
  virtual bool SelectedTextIsURL();                         \
  virtual bool IsCleanLinkCommand(int command_id) const;    \
  virtual void OnSanitizedCopy
#include "src/ui/views/controls/textfield/textfield_controller.h"
#undef OnAfterCutOrCopy

#endif  // BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_TEXTFIELD_TEXTFIELD_CONTROLLER_H_
