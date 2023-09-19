/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_UI_H_

#include <string>

#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"

namespace content {
class WebUI;
}

namespace webcompat_reporter {

class WebcompatReporterUI : public ConstrainedWebDialogUI {
 public:
  WebcompatReporterUI(content::WebUI* web_ui, const std::string& host);
  WebcompatReporterUI(const WebcompatReporterUI&) = delete;
  WebcompatReporterUI& operator=(const WebcompatReporterUI&) = delete;
  ~WebcompatReporterUI() override;
};

}  // namespace webcompat_reporter

#endif  // BRAVE_BROWSER_UI_WEBUI_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_UI_H_
