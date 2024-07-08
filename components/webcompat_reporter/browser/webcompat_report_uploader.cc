/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/webcompat_reporter/browser/webcompat_report_uploader.h"

#include <memory>
#include <string>
#include <utility>

#include "base/json/json_writer.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "brave/components/version_info/version_info.h"
#include "brave/components/webcompat_reporter/browser/fields.h"
#include "brave/components/webcompat_reporter/buildflags/buildflags.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/load_flags.h"
#include "net/base/mime_util.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/fetch_api.mojom-shared.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace webcompat_reporter {

namespace {
constexpr char kJsonContentType[] = "application/json";
constexpr char kPngContentType[] = "image/png";
constexpr char kMultipartContentTypePrefix[] = "multipart/form-data; boundary=";

constexpr char kReportDetailsMultipartName[] = "report-details";
constexpr char kScreenshotMultipartName[] = "screenshot";
constexpr char kScreenshotMultipartFilename[] = "screenshot.png";
}  // namespace

Report::Report() = default;
Report::~Report() = default;

WebcompatReportUploader::WebcompatReportUploader(
    scoped_refptr<network::SharedURLLoaderFactory> factory)
    : shared_url_loader_factory_(std::move(factory)) {}

WebcompatReportUploader::~WebcompatReportUploader() = default;

void WebcompatReportUploader::SubmitReport(const Report& report) {
  std::string api_key = brave_stats::GetAPIKey();

  const GURL upload_url(BUILDFLAG(WEBCOMPAT_REPORT_ENDPOINT));

  url::Origin report_url_origin = url::Origin::Create(report.report_url);

  base::Value::Dict report_details_dict;
  report_details_dict.Set(kSiteURLField, report.report_url.spec());
  report_details_dict.Set(kDomainField, report_url_origin.Serialize());
  report_details_dict.Set(kDetailsField, report.details.Clone());
  report_details_dict.Set(kContactField, report.contact.Clone());

  report_details_dict.Set(kChannelField, report.channel);
  report_details_dict.Set(
      kVersionField,
      version_info::GetBraveVersionWithoutChromiumMajorVersion());
  report_details_dict.Set(kShieldsEnabledField, report.shields_enabled);
  report_details_dict.Set(kAdBlockSettingField, report.ad_block_setting);
  report_details_dict.Set(kFPBlockSettingField, report.fp_block_setting);
  report_details_dict.Set(kAdBlockListsField, report.ad_block_list_names);
  report_details_dict.Set(kLanguagesField, report.languages);
  report_details_dict.Set(kLanguageFarblingField, report.language_farbling);
  report_details_dict.Set(kBraveVPNEnabledField, report.brave_vpn_connected);

  report_details_dict.Set(kApiKeyField, base::Value(api_key));

  std::string report_details_json;
  base::JSONWriter::Write(report_details_dict, &report_details_json);

  if (report.screenshot_png && !report.screenshot_png->empty()) {
    std::string multipart_boundary = net::GenerateMimeMultipartBoundary();
    std::string content_type = kMultipartContentTypePrefix + multipart_boundary;
    std::string multipart_data;

    net::AddMultipartValueForUpload(kReportDetailsMultipartName,
                                    report_details_json, multipart_boundary,
                                    kJsonContentType, &multipart_data);

    std::string screenshot_png_str;
    screenshot_png_str = std::string(report.screenshot_png->begin(),
                                     report.screenshot_png->end());
    net::AddMultipartValueForUploadWithFileName(
        kScreenshotMultipartName, kScreenshotMultipartFilename,
        screenshot_png_str, multipart_boundary, kPngContentType,
        &multipart_data);

    net::AddMultipartFinalDelimiterForUpload(multipart_boundary,
                                             &multipart_data);

    WebcompatReportUploader::CreateAndStartURLLoader(upload_url, content_type,
                                                     multipart_data);
    return;
  }

  WebcompatReportUploader::CreateAndStartURLLoader(upload_url, kJsonContentType,
                                                   report_details_json);
}

void WebcompatReportUploader::CreateAndStartURLLoader(
    const GURL& upload_url,
    const std::string& content_type,
    const std::string& post_data) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

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
                "Brave VPN connection status, user-provided additional details,"
                "optional screenshot and contact information."
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
