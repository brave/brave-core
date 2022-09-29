/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/pin_shortcut_handler.h"

#include "base/bind.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_ui.h"

#if BUILDFLAG(IS_WIN)
#include "brave/browser/brave_shell_integration_win.h"
#endif

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

#if BUILDFLAG(IS_WIN)
  shell_integration::win::PinToTaskbar(
      Profile::FromWebUI(web_ui()),
      base::BindOnce(&PinShortcutHandler::OnPinShortcut,
                     weak_factory_.GetWeakPtr()));
#endif
}

void PinShortcutHandler::HandleCheckShortcutPinState(
    const base::Value::List& args) {
  AllowJavascript();

#if BUILDFLAG(IS_WIN)
  shell_integration::win::IsShortcutPinned(
      base::BindOnce(&PinShortcutHandler::OnCheckShortcutPinState,
                     weak_factory_.GetWeakPtr()));
#endif
}

#if BUILDFLAG(IS_WIN)
void PinShortcutHandler::OnPinShortcut(bool pinned) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  NotifyShortcutPinStateChangeToPage(pinned);
}

void PinShortcutHandler::OnCheckShortcutPinState(bool pinned) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  NotifyShortcutPinStateChangeToPage(pinned);
}
#endif  // BUILDFLAG(IS_WIN)

void PinShortcutHandler::NotifyShortcutPinStateChangeToPage(bool pinned) {
  if (IsJavascriptAllowed()) {
    FireWebUIListener("shortcut-pin-state-changed", base::Value(pinned));
  }
}
