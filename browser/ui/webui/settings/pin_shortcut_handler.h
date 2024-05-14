/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_PIN_SHORTCUT_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_PIN_SHORTCUT_HANDLER_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "build/build_config.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

class PinShortcutHandler : public settings::SettingsPageUIHandler {
 public:
  PinShortcutHandler();
  ~PinShortcutHandler() override;

  PinShortcutHandler(const PinShortcutHandler&) = delete;
  PinShortcutHandler& operator=(const PinShortcutHandler&) = delete;

 private:
  // SettingsPageUIHandler overrides:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

  void HandleCheckShortcutPinState(const base::Value::List& args);
  void HandlePinShortcut(const base::Value::List& args);
  void NotifyShortcutPinStateChangeToPage(bool pinned);

  // |from_timer| is true when it's called for polling pinned state
  // after requesting pin. Only valid on Windows.
  void CheckShortcutPinState(bool from_timer);
  void OnPinShortcut(bool pinned);
  void OnCheckShortcutPinState(bool from_timer, bool pinned);

#if BUILDFLAG(IS_WIN)
  void OnPinStateCheckTimerFired();

  int pin_state_check_count_down_ = 0;
  std::unique_ptr<base::RetainingOneShotTimer> pin_state_check_timer_;
#endif

  base::WeakPtrFactory<PinShortcutHandler> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_PIN_SHORTCUT_HANDLER_H_
