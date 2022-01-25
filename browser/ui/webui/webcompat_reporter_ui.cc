/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/webcompat_reporter_ui.h"

#include <memory>
#include <utility>

#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/webcompat_reporter/browser/webcompat_report_uploader.h"
#include "brave/components/webcompat_reporter/resources/grit/webcompat_reporter_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace {

class WebcompatReporterDOMHandler : public content::WebUIMessageHandler {
 public:
  explicit WebcompatReporterDOMHandler(
      scoped_refptr<network::SharedURLLoaderFactory> loader_factory);
  WebcompatReporterDOMHandler(const WebcompatReporterDOMHandler&) = delete;
  WebcompatReporterDOMHandler& operator=(const WebcompatReporterDOMHandler&) =
      delete;
  ~WebcompatReporterDOMHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  void HandleSubmitReport(base::Value::ConstListView args);
  std::unique_ptr<brave::WebcompatReportUploader> uploader_;
};

WebcompatReporterDOMHandler::WebcompatReporterDOMHandler(
    scoped_refptr<network::SharedURLLoaderFactory> loader_factory)
    : uploader_(std::make_unique<brave::WebcompatReportUploader>(
          std::move(loader_factory))) {}

WebcompatReporterDOMHandler::~WebcompatReporterDOMHandler() = default;

void WebcompatReporterDOMHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "webcompat_reporter.submitReport",
      base::BindRepeating(&WebcompatReporterDOMHandler::HandleSubmitReport,
                          base::Unretained(this)));
}

void WebcompatReporterDOMHandler::HandleSubmitReport(
    base::Value::ConstListView args) {
  DCHECK_EQ(args.size(), 1U);
  if (!args[0].is_string())
    return;

  std::string site_url = args[0].GetString();
  uploader_->SubmitReport(site_url);
}

}  // namespace

WebcompatReporterUI::WebcompatReporterUI(content::WebUI* web_ui,
                                         const std::string& name)
    : ConstrainedWebDialogUI(web_ui) {
  CreateAndAddWebUIDataSource(web_ui, name, kWebcompatReporterGenerated,
                              kWebcompatReporterGeneratedSize,
                              IDR_WEBCOMPAT_REPORTER_HTML);
  Profile* profile = Profile::FromWebUI(web_ui);
  web_ui->AddMessageHandler(std::make_unique<WebcompatReporterDOMHandler>(
      profile->GetURLLoaderFactory()));
}

WebcompatReporterUI::~WebcompatReporterUI() {}
