// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WINDOW_CLOSING_CONFIRM_WINDOW_CLOSING_CONFIRM_CONTROLLER_H_
#define BRAVE_BROWSER_UI_WINDOW_CLOSING_CONFIRM_WINDOW_CLOSING_CONFIRM_CONTROLLER_H_

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

class BrowserWindowInterface;

// Asks users to confirm closing a browser window that has more than one tab,
// before any download warning or onbeforeunload handler runs.
class WindowClosingConfirmController {
 public:
  DECLARE_USER_DATA(WindowClosingConfirmController);

  explicit WindowClosingConfirmController(BrowserWindowInterface& browser);
  WindowClosingConfirmController(const WindowClosingConfirmController&) =
      delete;
  WindowClosingConfirmController& operator=(
      const WindowClosingConfirmController&) = delete;
  ~WindowClosingConfirmController();

  // Returns the instance owned by `browser`, or nullptr.
  static WindowClosingConfirmController* From(BrowserWindowInterface* browser);

  // Turns the confirmation off for every window while `suppress` is true.
  static void SuppressDialogForTesting(bool suppress);

  // Returns true when closing the window should be confirmed first.
  bool ShouldAskBeforeClosing() const;

  // Shows the confirmation dialog when closing should be confirmed, unless it
  // is already showing. Returns true while closing waits for the user.
  bool MaybeAskBeforeClosing();

 private:
  void OnDialogResponse(bool allowed_to_close);

  const raw_ref<BrowserWindowInterface> browser_;
  bool dialog_showing_ = false;
  ui::ScopedUnownedUserData<WindowClosingConfirmController>
      scoped_unowned_user_data_;
  base::WeakPtrFactory<WindowClosingConfirmController> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WINDOW_CLOSING_CONFIRM_WINDOW_CLOSING_CONFIRM_CONTROLLER_H_
