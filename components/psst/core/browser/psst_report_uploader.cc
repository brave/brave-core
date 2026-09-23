// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/core/browser/psst_report_uploader.h"

#include "base/json/json_writer.h"
#include "base/logging.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "brave/components/psst/buildflags/buildflags.h"
#include "net/base/load_flags.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace {
constexpr char kJsonContentType[] = "application/json";
inline constexpr char kChannelField[] = "channel";
inline constexpr char kVersionField[] = "version";
inline constexpr char kPsstCrxComponentVersionField[] =
    "psst_crx_component_version";
inline constexpr char kPsstScriptVersionField[] = "psst_script_version";
inline constexpr char kFailedTasksField[] = "failed_tasks";
inline constexpr char kApiKeyField[] = "api_key";
inline constexpr char kNotAvailable[] = "n/a";
}  // namespace

namespace psst {

PsstErrorReportUploader::PsstErrorReportUploader(
    scoped_refptr<network::SharedURLLoaderFactory> factory)
    : shared_url_loader_factory_(std::move(factory)) {}
PsstErrorReportUploader::~PsstErrorReportUploader() = default;

void PsstErrorReportUploader::Upload(
    std::optional<std::string> psst_component_version,
    const int script_version,
    const std::string& brave_version,
    std::optional<std::string> channel,
    base::ListValue failed_tasks,
    base::OnceCallback<void()> callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const GURL upload_url(BUILDFLAG(PSST_REPORT_ENDPOINT));
  base::DictValue report_details_dict;

  report_details_dict.Set(kApiKeyField, base::Value(brave_stats::GetAPIKey()));
  report_details_dict.Set(kVersionField, brave_version);
  report_details_dict.Set(kChannelField, channel.value_or(kNotAvailable));
  report_details_dict.Set(kPsstCrxComponentVersionField,
                          psst_component_version.value_or(kNotAvailable));
  report_details_dict.Set(kPsstScriptVersionField, script_version);
  report_details_dict.Set(kFailedTasksField,
                          base::Value(std::move(failed_tasks)));

  std::string report_details_json;
  base::JSONWriter::Write(report_details_dict, &report_details_json);

  CreateAndStartURLLoader(upload_url, kJsonContentType, report_details_json,
                          std::move(callback));
}

void PsstErrorReportUploader::CreateAndStartURLLoader(
    const GURL& upload_url,
    const std::string& content_type,
    const std::string& post_data,
    base::OnceCallback<void()> callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL(upload_url);
  resource_request->method = "POST";
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->load_flags =
      net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE;

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("psst_report_uploader", R"(
        semantics {
          sender: "Privacy Settings Selection for Sites Tool"
          description:
            "A user-initiated report of a website's privacy settings that
            the Privacy Settings Selection for Sites Tool couldn't apply"
          trigger:
            "The request is only triggered after the user clicks a
            button explicitly labeled to send a report."
          data: "Website where Privacy Settings Selection for Sites Tool
            failed, what step(s) failed, what the error(s) were.
            No additional user identifiers are sent."
          destination: OTHER
          destination_other: "Brave Privacy Settings Selection for Sites
            Tool Component Developers"
          last_reviewed: "2026-07-14"
          user_data {
            type: SENSITIVE_URL
          }
        }
        policy {
          cookies_allowed: NO
          policy_exception_justification:
            "Not implemented. This request is only sent when a user explicitly
            triggers it via the Privacy Settings Selection for Sites Tool
            consent dialog's 'Report a Broken PSST rules' option."
          setting:
            "Users can choose not to make this request by not sending
            the report via the tool. There's no explicit setting in the
            browser to stop this request once after the user has
            explicitly confirmed to send the failed report."
        })");

  simple_url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);
  simple_url_loader_->AttachStringForUpload(post_data, content_type);
  simple_url_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      shared_url_loader_factory_.get(),
      base::BindOnce(&PsstErrorReportUploader::OnSimpleURLLoaderComplete,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PsstErrorReportUploader::OnSimpleURLLoaderComplete(
    base::OnceCallback<void()> callback,
    std::optional<std::string> response_body) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const bool success = !!response_body;

  if (!success) {
    LOG(WARNING) << "Uploading psst report failed - please try again later!"
                 << std::endl;
  }

  if (callback) {
    std::move(callback).Run();
  }
}
}  // namespace psst
