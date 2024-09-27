// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/common/brave_channel_info.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/version_info/version_info.h"
#include "brave/components/webcompat_reporter/browser/webcompat_report_uploader.h"
#include "components/component_updater/component_updater_service.h"

namespace {

constexpr char kNotAvailable[] = "N/A";

void SetDictVal(std::optional<base::Value>& val_to_set,
                const std::optional<base::flat_map<std::string, std::string>>
                    opt_dict_val) {
  if (!opt_dict_val || opt_dict_val->empty()) {
    return;
  }
  auto detail_dict = base::Value::Dict();
  for (auto& detail_item : opt_dict_val.value()) {
    detail_dict.Set(detail_item.first, detail_item.second);
  }
  val_to_set = base::Value(std::move(detail_dict));
}

bool NeedsToGetComponentInfo(const std::string& component_id) {
  static const base::NoDestructor<std::unordered_set<std::string>>
      kComponentIds({
          "adcocjohghhfpidemphmcmlmhnfgikei",   // Brave Ad Block First Party
                                                    // Filters (plaintext)
          "bfpgedeaaibpoidldhjcknekahbikncb",   // Fanboy's Mobile Notifications
                                                    // (plaintext)
          "cdbbhgbmjhfnhnmgeddbliobbofkgdhe",   // EasyList Cookie (plaintext)
          "gkboaolpopklhgplhaaiboijnklogmbc",   // Regional Catalog
          "iodkpdagapdfkphljnddpjlldadblomo",   // Brave Ad Block Updater
                                                    // (plaintext)
          "jcfckfokjmopfomnoebdkdhbhcgjfnbi",   // Brave Experimental Adblock
                                                    // Rules (plaintext)
          brave_shields::kAdBlockResourceComponentId, // Brave Ad Block Updater
                                                          // (Resources)
      });
  return kComponentIds->contains(component_id);
}

void FillComponetsInfo(
    webcompat_reporter::Report& report,
    webcompat_reporter::mojom::ReportInfoPtr& report_info,
    raw_ptr<component_updater::ComponentUpdateService>& component_updater) {
  if (!report_info->ad_block_components_version) {
    report_info->ad_block_components_version =
        base::flat_map<std::string, std::string>();
  }

  auto components(component_updater->GetComponents());
  for (auto& ci : components) {
    if (!NeedsToGetComponentInfo(ci.id)) {
      continue;
    }
    report_info->ad_block_components_version.value().insert_or_assign(
        ci.id, base::StrCat({"Name: ", base::UTF16ToUTF8(ci.name), " Version: ",
                             ci.version.GetString(), " ID: ", ci.id}));
  }
  SetDictVal(report.ad_block_components,
             report_info->ad_block_components_version);
}

void FillReportVals(
    webcompat_reporter::Report& report,
    webcompat_reporter::mojom::ReportInfoPtr report_info,
    raw_ptr<component_updater::ComponentUpdateService>& component_updater) {
  report.channel = report_info->channel.value_or(brave::GetChannelName());
  report.brave_version =
      version_info::GetBraveVersionWithoutChromiumMajorVersion();
  report.report_url = GURL(report_info->report_url.value_or(kNotAvailable));
  report.shields_enabled = report_info->shields_enabled;
  report.ad_block_setting =
      report_info->ad_block_setting.value_or(kNotAvailable);
  report.fp_block_setting =
      report_info->fp_block_setting.value_or(kNotAvailable);
  report.ad_block_list_names = report_info->ad_block_list_names;
  report.languages = report_info->languages;
  report.language_farbling = report_info->language_farbling;
  report.brave_vpn_connected = report_info->brave_vpn_connected;
  SetDictVal(report.details, report_info->details);
  SetDictVal(report.contact, report_info->contact);
  FillComponetsInfo(report, report_info, component_updater);
  report.screenshot_png = report_info->screenshot_png;
}
}  // namespace
namespace webcompat_reporter {

WebcompatReporterService::WebcompatReporterService(
    component_updater::ComponentUpdateService* component_update_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : component_update_service_(component_update_service),
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
    mojom::ReportInfoPtr report_info,
    SubmitWebcompatReportCallback callback) {
  Report pending_report;
  FillReportVals(pending_report, std::move(report_info),
                 component_update_service_);
  SubmitReportInternal(std::move(pending_report));
  std::move(callback).Run();
}

void WebcompatReporterService::SubmitWebcompatReport(
    const Report& report_data) {
  base::ThreadPool::PostTask(
      FROM_HERE, {base::TaskPriority::BEST_EFFORT, base::MayBlock()},
      base::BindOnce(&WebcompatReporterService::SubmitReportInternal,
                     weak_factory_.GetWeakPtr(), std::move(report_data)));
}

void WebcompatReporterService::SubmitReportInternal(Report report_data) {
  if (!report_uploader_) {
    return;
  }
  report_uploader_->SubmitReport(report_data);
}
}  // namespace webcompat_reporter
