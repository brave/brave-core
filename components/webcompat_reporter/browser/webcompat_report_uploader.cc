/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/webcompat_reporter/browser/webcompat_report_uploader.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "brave/components/version_info/version_info.h"
#include "brave/components/webcompat_reporter/browser/fields.h"
#include "brave/components/webcompat_reporter/buildflags/buildflags.h"
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

constexpr char kStringTrue[] = "true";

constexpr char kComponentItemName[] = "name";
constexpr char kComponentItemId[] = "id";
constexpr char kComponentItemVersion[] = "version";

std::optional<base::Value> ConvertCompsToValue(
    const std::vector<webcompat_reporter::mojom::ComponentInfoPtr>&
        components) {
  if (components.empty()) {
    return std::nullopt;
  }
  auto components_list = base::Value::List();
  for (const auto& component : components) {
    base::Value::Dict component_dict;
    component_dict.Set(kComponentItemName, component->name);
    component_dict.Set(kComponentItemId, component->id);
    component_dict.Set(kComponentItemVersion, component->version);
    components_list.Append(std::move(component_dict));
  }
  return base::Value(std::move(components_list));
}

}  // namespace

WebcompatReportUploader::WebcompatReportUploader(
    scoped_refptr<network::SharedURLLoaderFactory> factory)
    : shared_url_loader_factory_(std::move(factory)) {}

WebcompatReportUploader::~WebcompatReportUploader() = default;

void WebcompatReportUploader::SubmitReport(mojom::ReportInfoPtr report_info) {
  if (!report_info) {
    return;
  }

  std::string api_key = brave_stats::GetAPIKey();

  const GURL upload_url(BUILDFLAG(WEBCOMPAT_REPORT_ENDPOINT));

  base::Value::Dict report_details_dict;

  if (report_info->report_url) {
    auto url = GURL(report_info->report_url.value());
    report_details_dict.Set(kDomainField, url::Origin::Create(url).Serialize());
    report_details_dict.Set(kSiteURLField,
                            GURL(report_info->report_url.value()).spec());
  }

  if (report_info->details) {
    report_details_dict.Set(kDetailsField, report_info->details.value());
  }

  if (report_info->ad_block_components_version &&
      !report_info->ad_block_components_version->empty()) {
    report_details_dict.Set(
        kAdBlockComponentsVersionField,
        ConvertCompsToValue(report_info->ad_block_components_version.value())
            .value());
  }

  if (report_info->contact) {
    report_details_dict.Set(kContactField, report_info->contact.value());
  }

  if (report_info->channel) {
    report_details_dict.Set(kChannelField, report_info->channel.value());
  }

  if (report_info->brave_version) {
    report_details_dict.Set(kVersionField, report_info->brave_version.value());
  }

  if (report_info->shields_enabled) {
    report_details_dict.Set(
        kShieldsEnabledField,
        report_info->shields_enabled.value() == kStringTrue);
  }

  if (report_info->ad_block_setting) {
    report_details_dict.Set(kAdBlockSettingField,
                            report_info->ad_block_setting.value());
  }

  if (report_info->fp_block_setting) {
    report_details_dict.Set(kFPBlockSettingField,
                            report_info->fp_block_setting.value());
  }

  if (report_info->ad_block_list_names) {
    report_details_dict.Set(kAdBlockListsField,
                            report_info->ad_block_list_names.value());
  }

  if (report_info->languages) {
    report_details_dict.Set(kLanguagesField, report_info->languages.value());
  }

  if (report_info->language_farbling) {
    report_details_dict.Set(
        kLanguageFarblingField,
        report_info->language_farbling.value() == kStringTrue);
  }

  if (report_info->brave_vpn_connected) {
    report_details_dict.Set(
        kBraveVPNEnabledField,
        report_info->brave_vpn_connected.value() == kStringTrue);
  }

  if (report_info->cookie_policy) {
    report_details_dict.Set(kCookiePolicyField,
                            report_info->cookie_policy.value());
  }

  report_details_dict.Set(kApiKeyField, base::Value(api_key));

  std::string report_details_json;
  base::JSONWriter::Write(report_details_dict, &report_details_json);

  if (report_info->screenshot_png && !report_info->screenshot_png->empty()) {
    std::string multipart_boundary = net::GenerateMimeMultipartBoundary();
    std::string content_type = kMultipartContentTypePrefix + multipart_boundary;
    std::string multipart_data;

    net::AddMultipartValueForUpload(kReportDetailsMultipartName,
                                    report_details_json, multipart_boundary,
                                    kJsonContentType, &multipart_data);

    std::string screenshot_png_str;
    screenshot_png_str = std::string(report_info->screenshot_png->begin(),
                                     report_info->screenshot_png->end());
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
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

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
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool success = !!response_body;

  if (success) {
    LOG(INFO) << "Successfully uploaded webcompat report. Thanks!" << std::endl;
  } else {
    LOG(ERROR) << "Uploading webcompat report failed - please try again later!"
               << std::endl;
  }
}

}  // namespace webcompat_reporter
