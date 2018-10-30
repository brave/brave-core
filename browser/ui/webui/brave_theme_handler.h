/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_THEME_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_THEME_HANDLER_H_

#include "content/public/browser/web_ui_message_handler.h"

class Profile;

class BraveThemeHandler : public content::WebUIMessageHandler {
 public:
  BraveThemeHandler() = default;
  ~BraveThemeHandler() override = default;

 private:
  // content::WebUIMessageHandler overrides:
  void RegisterMessages() override;

  void SetBraveThemeType(const base::ListValue* args);
  void GetBraveThemeType(const base::ListValue* args);

  Profile* profile_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(BraveThemeHandler);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_THEME_HANDLER_H_
