/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BASIC_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BASIC_UI_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"

class BasicUI : public content::WebUIController {
 public:
  explicit BasicUI(content::WebUI* web_ui, const std::string& host);
  ~BasicUI() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(BasicUI);
};

namespace basic_ui {

}  // namespace basic_ui

#endif  // BRAVE_BROWSER_UI_WEBUI_BASIC_UI_H_
