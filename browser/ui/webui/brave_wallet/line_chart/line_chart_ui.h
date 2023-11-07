/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_LINE_CHART_LINE_CHART_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_LINE_CHART_LINE_CHART_UI_H_

#include <memory>

#include "content/public/browser/web_ui.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/untrusted_web_ui_controller.h"

namespace line_chart {

class UntrustedLineChartUI : public ui::UntrustedWebUIController {
 public:
  explicit UntrustedLineChartUI(content::WebUI* web_ui);
  UntrustedLineChartUI(const UntrustedLineChartUI&) = delete;
  UntrustedLineChartUI& operator=(const UntrustedLineChartUI&) = delete;
  ~UntrustedLineChartUI() override;
};

class UntrustedLineChartUIConfig : public content::WebUIConfig {
 public:
  UntrustedLineChartUIConfig();
  ~UntrustedLineChartUIConfig() override = default;

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

}  // namespace line_chart

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_LINE_CHART_LINE_CHART_UI_H_
