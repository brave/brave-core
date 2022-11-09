/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_PIN_SHORTCUT_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_PIN_SHORTCUT_HANDLER_H_

#include "base/memory/weak_ptr.h"
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

  void OnPinShortcut(bool pinned);
  void OnCheckShortcutPinState(bool pinned);

  base::WeakPtrFactory<PinShortcutHandler> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_PIN_SHORTCUT_HANDLER_H_
