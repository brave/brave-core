/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_PSST_PSST_INTERNALS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_PSST_PSST_INTERNALS_UI_H_

#include "brave/browser/ui/webui/psst/psst_internals_handler.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/psst/core/common/psst_ui_common.mojom.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace psst {

class PsstInternalsUI;

class PsstInternalsUIConfig
    : public content::DefaultWebUIConfig<PsstInternalsUI> {
 public:
  PsstInternalsUIConfig()
      : DefaultWebUIConfig(content::kChromeUIScheme, kPsstInternalsHost) {}

  // There is no PSST log service - and so nothing to show - for
  // off-the-record profiles or when the feature is disabled.
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
};

// Diagnostics page showing the PSST flow timeline for this profile. Trusted
// chrome:// WebUI, so it is only reachable by typing the URL; web content can
// never navigate to it.
class PsstInternalsUI : public content::WebUIController {
 public:
  explicit PsstInternalsUI(content::WebUI* web_ui);
  PsstInternalsUI(const PsstInternalsUI&) = delete;
  PsstInternalsUI& operator=(const PsstInternalsUI&) = delete;
  ~PsstInternalsUI() override;

  void BindInterface(
      mojo::PendingReceiver<mojom::PsstInternalsHandler> receiver);

 private:
  PsstInternalsHandler handler_;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

}  // namespace psst

#endif  // BRAVE_BROWSER_UI_WEBUI_PSST_PSST_INTERNALS_UI_H_
