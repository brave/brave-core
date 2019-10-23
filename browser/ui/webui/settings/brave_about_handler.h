/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ABOUT_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ABOUT_HANDLER_H_

class Profile;

namespace content {
class WebUIDataSource;
}

namespace settings {

class AboutHandler;

class BraveAboutHandler {
 public:
  static AboutHandler* Create(content::WebUIDataSource* html_source,
                              Profile* profile);
};

}  // namespace settings

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ABOUT_HANDLER_H_
