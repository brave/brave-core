// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/values.h"
#include "brave/components/version_info/version_info.h"
#include "brave/components/webcompat_reporter/browser/webcompat_report_uploader.h"

namespace {
constexpr char kComponentItemName[] = "name";
constexpr char kComponentItemId[] = "id";
constexpr char kComponentItemVersion[] = "version";

void ConvertCompsToValue(
    std::optional<base::Value>& val_to_set,
    const std::vector<webcompat_reporter::mojom::ComponentInfoPtr>&
        components) {
  if (components.empty()) {
    return;
  }
  auto components_list = base::Value::List();
  for (const auto& component : components) {
    base::Value::Dict component_dict;
    component_dict.Set(kComponentItemName, component->name);
    component_dict.Set(kComponentItemId, component->id);
    component_dict.Set(kComponentItemVersion, component->version);
    components_list.Append(std::move(component_dict));
  }
  val_to_set = base::Value(std::move(components_list));
}

void FillReportWithAdblockListNames(
    webcompat_reporter::Report& report,
    webcompat_reporter::WebcompatReporterService::WebCompatServiceDelegate*
        service_delegate) {
  if (report.ad_block_list_names && !report.ad_block_list_names->empty()) {
    return;
  }

  if (!service_delegate) {
    return;
  }

  auto filter_list = service_delegate->GetAdblockFilterListNames();
  if (!filter_list) {
    return;
  }
  report.ad_block_list_names = base::JoinString(filter_list.value(), ",");
}

void FillReportWithComponetsInfo(
    webcompat_reporter::Report& report,
    webcompat_reporter::WebcompatReporterService::WebCompatServiceDelegate*
        service_delegate) {
  if (report.ad_block_components && !report.ad_block_components->is_none()) {
    return;
  }
  if (!service_delegate) {
    return;
  }

  auto component_infos = service_delegate->GetComponentInfos();
  if (!component_infos) {
    return;
  }

  auto components_list = base::Value::List();
  for (const auto& component : component_infos.value()) {
    base::Value::Dict component_dict;
    component_dict.Set(kComponentItemName, component.name);
    component_dict.Set(kComponentItemId, component.id);
    component_dict.Set(kComponentItemVersion, component.version);
    components_list.Append(std::move(component_dict));
  }
  report.ad_block_components = base::Value(std::move(components_list));
}

void FillReportByReportInfo(
    webcompat_reporter::Report& report,
    webcompat_reporter::mojom::ReportInfoPtr report_info) {
  if (!report_info) {
    return;
  }
  report.channel = report_info->channel;
  report.brave_version = report_info->brave_version;

  if (report_info && report_info->report_url &&
      !report_info->report_url->empty()) {
    report.report_url = GURL(report_info->report_url.value());
  } else {
    report.report_url = std::nullopt;
  }

  if (report_info->ad_block_components_version) {
    ConvertCompsToValue(report.ad_block_components,
                        report_info->ad_block_components_version.value());
  }

  if (report_info->ad_block_list_names &&
      !report_info->ad_block_list_names->empty()) {
    report.ad_block_list_names = report_info->ad_block_list_names;
  }

  report.shields_enabled = report_info->shields_enabled;
  report.ad_block_setting = report_info->ad_block_setting;
  report.fp_block_setting = report_info->fp_block_setting;
  report.languages = report_info->languages;
  report.language_farbling = report_info->language_farbling;
  report.brave_vpn_connected = report_info->brave_vpn_connected;
  report.details = report_info->details;
  report.contact = report_info->contact;
  report.screenshot_png = report_info->screenshot_png;
}

void FillReportValues(
    webcompat_reporter::Report& report,
    webcompat_reporter::WebcompatReporterService::WebCompatServiceDelegate*
        service_delegate) {
  if (!report.channel && service_delegate) {
    report.channel = service_delegate->GetChannelName();
  }

  if (!report.brave_version) {
    report.brave_version =
        version_info::GetBraveVersionWithoutChromiumMajorVersion();
  }
  FillReportWithComponetsInfo(report, service_delegate);
  FillReportWithAdblockListNames(report, service_delegate);
}
}  // namespace
namespace webcompat_reporter {

WebcompatReporterService::WebcompatReporterService() = default;

WebcompatReporterService::WebcompatReporterService(
    std::unique_ptr<WebCompatServiceDelegate> service_delegate,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : service_delegate_(std::move(service_delegate)),
      report_uploader_(
          std::make_unique<WebcompatReportUploader>(url_loader_factory)) {}

WebcompatReporterService::~WebcompatReporterService() = default;

mojo::PendingRemote<mojom::WebcompatReporterHandler>
WebcompatReporterService::MakeRemote() {
  mojo::PendingRemote<mojom::WebcompatReporterHandler> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void WebcompatReporterService::Bind(
    mojo::PendingReceiver<mojom::WebcompatReporterHandler> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void WebcompatReporterService::SubmitWebcompatReport(
    mojom::ReportInfoPtr report_info) {
  Report pending_report;
  FillReportByReportInfo(pending_report, std::move(report_info));
  SubmitWebcompatReport(pending_report);
}

void WebcompatReporterService::SubmitWebcompatReport(Report report_data) {
  FillReportValues(report_data,
                   service_delegate_ ? service_delegate_.get() : nullptr);
  SubmitReportInternal(report_data);
}

void WebcompatReporterService::SubmitReportInternal(const Report& report_data) {
  if (!report_uploader_) {
    return;
  }
  report_uploader_->SubmitReport(report_data);
}

void WebcompatReporterService::SetReportUploaderForTest(
    std::unique_ptr<WebcompatReportUploader> report_uploader) {
  report_uploader_ = std::move(report_uploader);
}
void WebcompatReporterService::SetDelegateForTest(
    std::unique_ptr<WebCompatServiceDelegate> service_delegate) {
  service_delegate_ = std::move(service_delegate);
}
}  // namespace webcompat_reporter
