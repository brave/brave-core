/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/webcompat_reporter/browser/webcompat_report_uploader.h"

#include <memory>
#include <string>
#include <utility>

#include "base/environment.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "brave/components/webcompat_reporter/browser/fields.h"
#include "brave/components/webcompat_reporter/buildflags/buildflags.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/load_flags.h"
#include "net/base/privacy_mode.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/fetch_api.mojom-shared.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace webcompat_reporter {

WebcompatReportUploader::WebcompatReportUploader(
    scoped_refptr<network::SharedURLLoaderFactory> factory)
    : shared_url_loader_factory_(std::move(factory)) {}

WebcompatReportUploader::~WebcompatReportUploader() = default;

void WebcompatReportUploader::SubmitReport(
    const GURL& report_url,
    const bool shields_enabled,
    const std::string& ad_block_setting,
    const std::string& fp_block_setting,
    const std::string& ad_block_list_names,
    const std::string& languages,
    const bool language_farbling,
    const bool brave_vpn_connected,
    const base::Value& details,
    const base::Value& contact) {
  std::string api_key = brave_stats::GetAPIKey();

  const GURL upload_url(BUILDFLAG(WEBCOMPAT_REPORT_ENDPOINT));

  url::Origin report_url_origin = url::Origin::Create(report_url);

  base::Value::Dict post_data_obj;
  post_data_obj.Set(kSiteURLField, report_url.spec());
  post_data_obj.Set(kDomainField, report_url_origin.Serialize());
  post_data_obj.Set(kDetailsField, details.Clone());
  post_data_obj.Set(kContactField, contact.Clone());

  post_data_obj.Set(kShieldsEnabledField, shields_enabled);
  post_data_obj.Set(kAdBlockSettingField, ad_block_setting);
  post_data_obj.Set(kFPBlockSettingField, fp_block_setting);
  post_data_obj.Set(kAdBlockListsField, ad_block_list_names);
  post_data_obj.Set(kLanguagesField, languages);
  post_data_obj.Set(kLanguageFarblingField, language_farbling);
  post_data_obj.Set(kBraveVPNEnabledField, brave_vpn_connected);

  post_data_obj.Set(kApiKeyField, base::Value(api_key));

  std::string post_data;
  base::JSONWriter::Write(post_data_obj, &post_data);

  WebcompatReportUploader::CreateAndStartURLLoader(upload_url, post_data);
}

void WebcompatReportUploader::CreateAndStartURLLoader(
    const GURL& upload_url,
    const std::string& post_data) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::string content_type = "application/json";
  auto resource_request = std::make_unique<network::ResourceRequest>();
  // upload_url only includes the origin and path, and not the fragment or
  // query. The fragment and query are removed from the URL in
  // OpenReporterDialog.
  resource_request->url = GURL(upload_url);
  resource_request->method = "POST";
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->load_flags =
      net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE;

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("background_performance_tracer", R"(
        semantics {
          sender: "Brave Web Compatibility Reporting"
          description:
            "A user-initiated report of a website that appears broken as a"
            "result of having Brave Shields enabled."
          trigger:
            "Though the 'Report a Broken Site' option of the help menu or"
            "the Brave Shields panel."
          data: "Broken URL, IP address, Shields settings, language settings,"
                "Brave VPN connection status; user provided additional details"
                "and contact information."
          destination: OTHER
          destination_other: "Brave developers"
        }
        policy {
          cookies_allowed: NO
        })");

  simple_url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);
  simple_url_loader_->AttachStringForUpload(post_data, content_type);
  simple_url_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      shared_url_loader_factory_.get(),
      base::BindOnce(&WebcompatReportUploader::OnSimpleURLLoaderComplete,
                     base::Unretained(this)));
}

void WebcompatReportUploader::OnSimpleURLLoaderComplete(
    std::unique_ptr<std::string> response_body) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  bool success = !!response_body;

  if (success) {
    LOG(INFO) << "Successfully uploaded webcompat report. Thanks!" << std::endl;
  } else {
    LOG(ERROR) << "Uploading webcompat report failed - please try again later!"
               << std::endl;
  }
}

}  // namespace webcompat_reporter
