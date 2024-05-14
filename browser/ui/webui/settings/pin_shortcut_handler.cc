/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/pin_shortcut_handler.h"

#include "base/functional/bind.h"
#include "brave/browser/brave_shell_integration.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_ui.h"

PinShortcutHandler::PinShortcutHandler() = default;

PinShortcutHandler::~PinShortcutHandler() = default;

void PinShortcutHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "checkShortcutPinState",
      base::BindRepeating(&PinShortcutHandler::HandleCheckShortcutPinState,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "pinShortcut", base::BindRepeating(&PinShortcutHandler::HandlePinShortcut,
                                         base::Unretained(this)));
}

void PinShortcutHandler::HandlePinShortcut(const base::Value::List& args) {
  AllowJavascript();

  shell_integration::PinShortcut(
      Profile::FromWebUI(web_ui()),
      base::BindOnce(&PinShortcutHandler::OnPinShortcut,
                     weak_factory_.GetWeakPtr()));
}

void PinShortcutHandler::HandleCheckShortcutPinState(
    const base::Value::List& args) {
  AllowJavascript();

  CheckShortcutPinState(/*from_timer*/ false);
}

void PinShortcutHandler::CheckShortcutPinState(bool from_timer) {
  shell_integration::IsShortcutPinned(
      base::BindOnce(&PinShortcutHandler::OnCheckShortcutPinState,
                     weak_factory_.GetWeakPtr(), from_timer));
}

#if BUILDFLAG(IS_WIN)
void PinShortcutHandler::OnPinStateCheckTimerFired() {
  CheckShortcutPinState(/*from_timer*/ true);
}
#endif

void PinShortcutHandler::OnPinShortcut(bool pinned) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!pinned) {
    return;
  }

#if BUILDFLAG(IS_WIN)
  // On Windows, ignore |pinned| value as Windows asks to user via another
  // os notification to pin. |pinned| state just represents that api call is
  // succuss. So, polling to check whether user allowed or not.
  if (!pin_state_check_timer_) {
    pin_state_check_timer_ = std::make_unique<base::RetainingOneShotTimer>(
        FROM_HERE, base::Seconds(2),
        base::BindRepeating(&PinShortcutHandler::OnPinStateCheckTimerFired,
                            weak_factory_.GetWeakPtr()));
  }
  // We'll try to check 10 times after asking to pin to Windows.
  // Maybe user will see os notification for pin right after clicking the Pin
  // button. If user doesn't click allow in 20s, we'll not monitor anymore as we
  // could give proper pin state after reloading.
  pin_state_check_count_down_ = 10;
  pin_state_check_timer_->Reset();
#else
  NotifyShortcutPinStateChangeToPage(pinned);
#endif
}

void PinShortcutHandler::OnCheckShortcutPinState(bool from_timer, bool pinned) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

#if BUILDFLAG(IS_WIN)
  if (from_timer) {
    pin_state_check_count_down_--;
    // Clear if pinned or we did all try.
    if (pinned || pin_state_check_count_down_ == 0) {
      pin_state_check_count_down_ = 0;
      pin_state_check_timer_.reset();
    } else {
      // Check again.
      pin_state_check_timer_->Reset();
    }
  }
#else
  CHECK(!from_timer);
#endif
  NotifyShortcutPinStateChangeToPage(pinned);
}

void PinShortcutHandler::NotifyShortcutPinStateChangeToPage(bool pinned) {
  if (IsJavascriptAllowed()) {
    FireWebUIListener("shortcut-pin-state-changed", base::Value(pinned));
  }
}
