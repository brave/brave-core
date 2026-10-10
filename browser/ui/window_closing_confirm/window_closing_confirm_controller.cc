// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/window_closing_confirm/window_closing_confirm_controller.h"

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/types/to_address.h"
#include "brave/browser/ui/window_closing_confirm/window_closing_confirm_dialog_view.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/lifetime/browser_close_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/unload_controller.h"
#include "components/prefs/pref_service.h"

namespace {

bool g_suppress_dialog_for_testing = false;

}  // namespace

DEFINE_USER_DATA(WindowClosingConfirmController);

WindowClosingConfirmController::WindowClosingConfirmController(
    BrowserWindowInterface& browser)
    : browser_(browser),
      scoped_unowned_user_data_(browser.GetUnownedUserDataHost(), *this) {}

WindowClosingConfirmController::~WindowClosingConfirmController() = default;

// static
WindowClosingConfirmController* WindowClosingConfirmController::From(
    BrowserWindowInterface* browser) {
  return Get(browser->GetUnownedUserDataHost());
}

// static
void WindowClosingConfirmController::SuppressDialogForTesting(bool suppress) {
  g_suppress_dialog_for_testing = suppress;
}

bool WindowClosingConfirmController::ShouldAskBeforeClosing() const {
  if (g_suppress_dialog_for_testing) {
    CHECK_IS_TEST();
    return false;
  }

  // Don't need to ask when application closing is in-progress.
  if (BrowserCloseManager::BrowserClosingStarted()) {
    return false;
  }

  if (UnloadController::From(base::to_address(browser_))
          ->confirmed_to_close()) {
    return false;
  }

  PrefService* prefs = browser_->GetProfile()->GetPrefs();
  if (!prefs->GetBoolean(kEnableWindowClosingConfirm)) {
    return false;
  }

  // Only launch confirm dialog while closing when browser has multiple tabs.
  return browser_->GetTabStripModel()->count() > 1;
}

bool WindowClosingConfirmController::MaybeAskBeforeClosing() {
  if (!ShouldAskBeforeClosing()) {
    return false;
  }

  if (!dialog_showing_) {
    dialog_showing_ = true;
    WindowClosingConfirmDialogView::Show(
        base::to_address(browser_),
        base::BindOnce(&WindowClosingConfirmController::OnDialogResponse,
                       weak_ptr_factory_.GetWeakPtr()));
  }
  return true;
}

void WindowClosingConfirmController::OnDialogResponse(bool allowed_to_close) {
  CHECK(dialog_showing_);
  dialog_showing_ = false;

  // Record the user's choice on the window-scoped UnloadController, which
  // tracks the result of any warning or beforeunload handlers.
  UnloadController::From(base::to_address(browser_))
      ->set_confirmed_to_close(allowed_to_close);
  if (allowed_to_close) {
    // Start closing the window again now that the user allowed it. The
    // confirmation is not shown for this request because
    // UnloadController::confirmed_to_close() is set. If a later warning or
    // beforeunload handler cancels closing, it is shown again.
    chrome::CloseWindow(base::to_address(browser_));
  }
}
