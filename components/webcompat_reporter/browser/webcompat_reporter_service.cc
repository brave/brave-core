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

struct ReportFiller {
  ReportFiller(
      webcompat_reporter::mojom::ReportInfoPtr& report_info,
      webcompat_reporter::WebcompatReporterService::Delegate* service_delegate)
      : report_info(report_info), service_delegate(service_delegate) {}
  ~ReportFiller() = default;

  ReportFiller& FillReportWithAdblockListNames() {
    if ((*report_info)->ad_block_list_names &&
        !(*report_info)->ad_block_list_names->empty()) {
      return *this;
    }

    if (!service_delegate) {
      return *this;
    }

    auto filter_list = service_delegate->GetAdblockFilterListNames();
    if (!filter_list) {
      return *this;
    }
    (*report_info)->ad_block_list_names =
        base::JoinString(filter_list.value(), ",");
    return *this;
  }

  ReportFiller& FillReportWithComponetsInfo() {
    if ((*report_info)->ad_block_components_version &&
        !(*report_info)->ad_block_components_version->empty()) {
      return *this;
    }
    if (!service_delegate) {
      return *this;
    }

    auto component_infos = service_delegate->GetComponentInfos();
    if (!component_infos) {
      return *this;
    }

    std::vector<webcompat_reporter::mojom::ComponentInfoPtr> components_list;
    for (const auto& component : component_infos.value()) {
      components_list.emplace_back(
          webcompat_reporter::mojom::ComponentInfo::New(
              component.name, component.id, component.version));
    }
    (*report_info)->ad_block_components_version = std::move(components_list);
    return *this;
  }

  ReportFiller& FillChannel() {
    if (!(*report_info)->channel && service_delegate) {
      (*report_info)->channel = service_delegate->GetChannelName();
    }
    return *this;
  }

  ReportFiller& FillVersion() {
    if (!(*report_info)->brave_version) {
      (*report_info)->brave_version =
          version_info::GetBraveVersionWithoutChromiumMajorVersion();
    }
    return *this;
  }

  raw_ref<webcompat_reporter::mojom::ReportInfoPtr> report_info;
  const raw_ptr<webcompat_reporter::WebcompatReporterService::Delegate>
      service_delegate;
};

}  // namespace

namespace webcompat_reporter {

WebcompatReporterService::WebcompatReporterService(
    std::unique_ptr<Delegate> service_delegate,
    std::unique_ptr<WebcompatReportUploader> report_uploader)
    : service_delegate_(std::move(service_delegate)),
      report_uploader_(std::move(report_uploader)) {}

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
  ReportFiller(report_info,
               service_delegate_ ? service_delegate_.get() : nullptr)
      .FillChannel()
      .FillVersion()
      .FillReportWithComponetsInfo()
      .FillReportWithAdblockListNames();

  report_uploader_->SubmitReport(std::move(report_info));
}

}  // namespace webcompat_reporter
