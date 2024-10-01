// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/common/brave_channel_info.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"
#include "brave/components/brave_shields/core/browser/filter_list_catalog_entry.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/version_info/version_info.h"
#include "brave/components/webcompat_reporter/browser/webcompat_report_uploader.h"
#include "components/component_updater/component_updater_service.h"

namespace {
constexpr char kComponentItemName[] = "name";
constexpr char kComponentItemId[] = "id";
constexpr char kComponentItemVersion[] = "version";

void SetDictVal(std::optional<base::Value>& val_to_set,
                const std::optional<base::flat_map<std::string, std::string>>&
                    opt_dict_val) {
  if (!opt_dict_val || opt_dict_val->empty()) {
    return;
  }
  auto detail_dict = base::Value::Dict();
  for (const auto& detail_item : opt_dict_val.value()) {
    detail_dict.Set(detail_item.first, detail_item.second);
  }
  val_to_set = base::Value(std::move(detail_dict));
}
void SetListVal(
    std::optional<base::Value>& val_to_set,
    const base::flat_map<std::string, base::Value::Dict>& opt_dict_val) {
  if (opt_dict_val.empty()) {
    return;
  }
  auto list = base::Value::List();
  for (const auto& item : opt_dict_val) {
    list.Append(item.second.Clone());
  }
  val_to_set = base::Value(std::move(list));
}
void ConvertCompsToValue(
    std::optional<base::Value>& val_to_set,
    const std::vector<webcompat_reporter::mojom::ComponentInfoPtr>& components) {
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
bool NeedsToGetComponentInfo(const std::string& component_id) {
  static const base::NoDestructor<std::unordered_set<std::string>>
      kComponentIds({
          "adcocjohghhfpidemphmcmlmhnfgikei",  // Brave Ad Block First Party
                                               // Filters (plaintext)
          "bfpgedeaaibpoidldhjcknekahbikncb",  // Fanboy's Mobile Notifications
                                               // (plaintext)
          "cdbbhgbmjhfnhnmgeddbliobbofkgdhe",  // EasyList Cookie (plaintext)
          "gkboaolpopklhgplhaaiboijnklogmbc",  // Regional Catalog
          "iodkpdagapdfkphljnddpjlldadblomo",  // Brave Ad Block Updater
                                               // (plaintext)
          "jcfckfokjmopfomnoebdkdhbhcgjfnbi",  // Brave Experimental Adblock
                                               // Rules (plaintext)
          brave_shields::kAdBlockResourceComponentId,  // Brave Ad Block Updater
                                                       // (Resources)
      });
  return kComponentIds->contains(component_id);
}

void FillReportWithAdblockListNames(
    webcompat_reporter::Report& report,
    raw_ptr<brave_shields::AdBlockService>& ad_block_service) {
  if (report.ad_block_list_names && !report.ad_block_list_names->empty()) {
    return;
  }

  if (!ad_block_service) {
    return;
  }

  std::vector<std::string> ad_block_list_names;
  if (ad_block_service != nullptr) {
    brave_shields::AdBlockComponentServiceManager* service_manager =
        ad_block_service->component_service_manager();
    CHECK(service_manager);
    for (const brave_shields::FilterListCatalogEntry& entry :
         service_manager->GetFilterListCatalog()) {
      if (service_manager->IsFilterListEnabled(entry.uuid)) {
        ad_block_list_names.push_back(entry.title);
      }
    }
  }
  report.ad_block_list_names = base::JoinString(ad_block_list_names, ",");
}

void FillReportWithComponetsInfo(
    webcompat_reporter::Report& report,
    raw_ptr<component_updater::ComponentUpdateService>& component_updater) {
  if (report.ad_block_components && !report.ad_block_components->is_none()) {
    return;
  }
  if (!component_updater) {
    return;
  }
  base::flat_map<std::string, base::Value::Dict> ad_block_components_version;
  auto components(component_updater->GetComponents());
  for (const auto& ci : components) {
    if (!NeedsToGetComponentInfo(ci.id)) {
      continue;
    }
    base::Value::Dict dict;
    dict.Set(kComponentItemName, base::UTF16ToUTF8(ci.name));
    dict.Set(kComponentItemId, ci.id);
    dict.Set(kComponentItemVersion, ci.version.GetString());

    ad_block_components_version.insert_or_assign(ci.id, std::move(dict));
  }
  if (ad_block_components_version.empty()) {
    return;
  }

  SetListVal(report.ad_block_components, ad_block_components_version);
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
  SetDictVal(report.details, report_info->details);
  SetDictVal(report.contact, report_info->contact);
  report.screenshot_png = report_info->screenshot_png;
}

void FillReportValues(
    webcompat_reporter::Report& report,
    raw_ptr<component_updater::ComponentUpdateService>& component_updater,
    raw_ptr<brave_shields::AdBlockService>& ad_block_service) {
  if (!report.channel) {
    report.channel = brave::GetChannelName();
  }
  if (!report.brave_version) {
    report.brave_version =
        version_info::GetBraveVersionWithoutChromiumMajorVersion();
  }
  FillReportWithComponetsInfo(report, component_updater);
  FillReportWithAdblockListNames(report, ad_block_service);
}
}  // namespace
namespace webcompat_reporter {

WebcompatReporterService::WebcompatReporterService() = default;

WebcompatReporterService::WebcompatReporterService(
    brave_shields::AdBlockService* adblock_service,
    component_updater::ComponentUpdateService* component_update_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : component_update_service_(component_update_service),
      adblock_service_(adblock_service),
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
  FillReportByReportInfo(pending_report, std::move(report_info));
  SubmitWebcompatReport(pending_report);
  std::move(callback).Run();
}

void WebcompatReporterService::SubmitWebcompatReport(Report report_data) {
  FillReportValues(report_data, component_update_service_, adblock_service_);
  SubmitReportInternal(report_data);
}

void WebcompatReporterService::SubmitReportInternal(const Report& report_data) {
  if (!report_uploader_) {
    return;
  }
  report_uploader_->SubmitReport(report_data);
}

void WebcompatReporterService::SetUpWebcompatReporterServiceForTest(
    std::unique_ptr<WebcompatReportUploader> report_uploader,
    component_updater::ComponentUpdateService* component_update_service) {
  component_update_service_ = component_update_service;
  report_uploader_ = std::move(report_uploader);
}
}  // namespace webcompat_reporter
