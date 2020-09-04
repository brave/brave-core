// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_UI_H_

#include <string>

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"

class InstantService;
class Profile;

class BraveNewTabUI : public content::WebUIController {
 public:
  BraveNewTabUI(content::WebUI* web_ui, const std::string& name);
  ~BraveNewTabUI() override;
 private:
  Profile* profile_;
  InstantService* instant_service_;

  DISALLOW_COPY_AND_ASSIGN(BraveNewTabUI);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_UI_H_
