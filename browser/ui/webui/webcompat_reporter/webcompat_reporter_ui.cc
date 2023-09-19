/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/webcompat_reporter/webcompat_reporter_ui.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/strings/string_util.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/ui/brave_shields_data_controller.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/filter_list_catalog_entry.h"
#include "brave/components/brave_shields/common/pref_names.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/webcompat_reporter/browser/fields.h"
#include "brave/components/webcompat_reporter/browser/webcompat_report_uploader.h"
#include "brave/components/webcompat_reporter/resources/grit/webcompat_reporter_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "components/language/core/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "ui/web_dialogs/web_dialog_delegate.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#endif

namespace webcompat_reporter {

namespace {

class WebcompatReporterDOMHandler : public content::WebUIMessageHandler {
 public:
  explicit WebcompatReporterDOMHandler(Profile* profile);
  WebcompatReporterDOMHandler(const WebcompatReporterDOMHandler&) = delete;
  WebcompatReporterDOMHandler& operator=(const WebcompatReporterDOMHandler&) =
      delete;
  ~WebcompatReporterDOMHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  void InitAdditionalParameters(Profile* profile);
  void HandleSubmitReport(const base::Value::List& args);

  std::unique_ptr<webcompat_reporter::WebcompatReportUploader> uploader_;
  std::string ad_block_list_names_;
  std::string languages_;
  bool language_farbling_enabled_ = true;
  bool brave_vpn_connected_ = false;
};

WebcompatReporterDOMHandler::WebcompatReporterDOMHandler(Profile* profile)
    : uploader_(std::make_unique<webcompat_reporter::WebcompatReportUploader>(
          profile->GetURLLoaderFactory())) {
  InitAdditionalParameters(profile);
}

void WebcompatReporterDOMHandler::InitAdditionalParameters(Profile* profile) {
  std::vector<std::string> ad_block_list_names;

  // Collect all enabled adblock list names
  brave_shields::AdBlockService* ad_block_service =
      g_brave_browser_process->ad_block_service();
  if (ad_block_service != nullptr) {
    brave_shields::AdBlockRegionalServiceManager* regional_service_manager =
        ad_block_service->regional_service_manager();
    CHECK(regional_service_manager);
    for (const brave_shields::FilterListCatalogEntry& entry :
         regional_service_manager->GetFilterListCatalog()) {
      if (regional_service_manager->IsFilterListEnabled(entry.uuid)) {
        ad_block_list_names.push_back(entry.title);
      }
    }
  }

  ad_block_list_names_ = base::JoinString(ad_block_list_names, ",");

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  brave_vpn::BraveVpnService* vpn_service =
      brave_vpn::BraveVpnServiceFactory::GetForProfile(profile);
  if (vpn_service != nullptr) {
    brave_vpn_connected_ = vpn_service->IsConnected();
  }
#endif

  PrefService* profile_prefs = profile->GetPrefs();
  languages_ = profile_prefs->GetString(language::prefs::kAcceptLanguages);
  language_farbling_enabled_ =
      profile_prefs->GetBoolean(brave_shields::prefs::kReduceLanguageEnabled);
}

WebcompatReporterDOMHandler::~WebcompatReporterDOMHandler() = default;

void WebcompatReporterDOMHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "webcompat_reporter.submitReport",
      base::BindRepeating(&WebcompatReporterDOMHandler::HandleSubmitReport,
                          base::Unretained(this)));
}

void WebcompatReporterDOMHandler::HandleSubmitReport(
    const base::Value::List& args) {
  DCHECK_EQ(args.size(), 1U);
  if (!args[0].is_dict()) {
    return;
  }

  const base::Value::Dict& submission_args = args[0].GetDict();

  const std::string* url_arg = submission_args.FindString(kSiteURLField);
  const std::string* ad_block_setting_arg =
      submission_args.FindString(kAdBlockSettingField);
  const std::string* fp_block_setting_arg =
      submission_args.FindString(kFPBlockSettingField);
  const base::Value* details_arg = submission_args.Find(kDetailsField);
  const base::Value* contact_arg = submission_args.Find(kContactField);
  bool shields_enabled =
      submission_args.FindBool(kShieldsEnabledField).value_or(false);

  std::string url;
  std::string ad_block_setting;
  std::string fp_block_setting;
  base::Value details;
  base::Value contact;

  if (url_arg != nullptr) {
    url = *url_arg;
  }
  if (ad_block_setting_arg != nullptr) {
    ad_block_setting = *ad_block_setting_arg;
  }
  if (fp_block_setting_arg != nullptr) {
    fp_block_setting = *fp_block_setting_arg;
  }
  if (details_arg != nullptr) {
    details = details_arg->Clone();
  }
  if (contact_arg != nullptr) {
    contact = contact_arg->Clone();
  }

  uploader_->SubmitReport(GURL(url), shields_enabled, ad_block_setting,
                          fp_block_setting, ad_block_list_names_, languages_,
                          language_farbling_enabled_, brave_vpn_connected_,
                          details, contact);
}

}  // namespace

WebcompatReporterUI::WebcompatReporterUI(content::WebUI* web_ui,
                                         const std::string& name)
    : ConstrainedWebDialogUI(web_ui) {
  CreateAndAddWebUIDataSource(web_ui, name, kWebcompatReporterGenerated,
                              kWebcompatReporterGeneratedSize,
                              IDR_WEBCOMPAT_REPORTER_HTML);
  Profile* profile = Profile::FromWebUI(web_ui);

  web_ui->AddMessageHandler(
      std::make_unique<WebcompatReporterDOMHandler>(profile));
}

WebcompatReporterUI::~WebcompatReporterUI() = default;

}  // namespace webcompat_reporter
