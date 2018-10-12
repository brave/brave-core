// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_DONATE_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_DONATE_UI_H_

#include "base/macros.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"

class BraveDonateUI : public ConstrainedWebDialogUI {
 public:
  BraveDonateUI(content::WebUI* web_ui, const std::string& host);
  ~BraveDonateUI() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveDonateUI);
};

#endif
