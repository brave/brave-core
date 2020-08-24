/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/webcompat_reporter_ui.h"

#include <memory>

#include "brave/browser/ui/webui/basic_ui.h"
#include "brave/common/brave_webui_url_constants.h"
#include "brave/components/webcompat_reporter/resources/grit/webcompat_reporter_generated_map.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

#include "brave/components/webcompat_reporter/browser/webcompat_report_uploader.h"

namespace {

class WebcompatReporterDOMHandler : public content::WebUIMessageHandler {
 public:
  WebcompatReporterDOMHandler();
  ~WebcompatReporterDOMHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  void HandleSubmitReport(const base::ListValue* args);
  std::unique_ptr<brave::WebcompatReportUploader> uploader_;

  DISALLOW_COPY_AND_ASSIGN(WebcompatReporterDOMHandler);
};

WebcompatReporterDOMHandler::WebcompatReporterDOMHandler()
    : uploader_(std::make_unique<brave::WebcompatReportUploader>(
          g_browser_process->shared_url_loader_factory())) {}

WebcompatReporterDOMHandler::~WebcompatReporterDOMHandler() {}

void WebcompatReporterDOMHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "webcompat_reporter.submitReport",
      base::BindRepeating(&WebcompatReporterDOMHandler::HandleSubmitReport,
                          base::Unretained(this)));
}

void WebcompatReporterDOMHandler::HandleSubmitReport(
    const base::ListValue* args) {
  DCHECK_EQ(args->GetSize(), 1U);
  std::string site_url;
  if (!args->GetString(0, &site_url))
    return;

  uploader_->SubmitReport(site_url);
}

}  // namespace

WebcompatReporterUI::WebcompatReporterUI(content::WebUI* web_ui,
                                         const std::string& name)
    : ConstrainedWebDialogUI(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource* data_source = CreateBasicUIHTMLSource(
      profile, name, kWebcompatReporterGenerated,
      kWebcompatReporterGeneratedSize, IDR_WEBCOMPAT_REPORTER_HTML);
  content::WebUIDataSource::Add(profile, data_source);

  web_ui->AddMessageHandler(std::make_unique<WebcompatReporterDOMHandler>());
}

WebcompatReporterUI::~WebcompatReporterUI() {}
