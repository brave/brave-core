// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_WEBCOMPAT_REPORTER_SERVICE_H_
#define BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_WEBCOMPAT_REPORTER_SERVICE_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/webcompat_reporter/browser/webcompat_report_uploader.h"
#include "brave/components/webcompat_reporter/common/webcompat_reporter.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace brave_shields {
class AdBlockService;
}  // namespace brave_shields

namespace webcompat_reporter {

// This class is not thread-safe and should have single owner
class WebcompatReporterService : public KeyedService,
                                 public mojom::WebcompatReporterHandler {
 public:
  explicit WebcompatReporterService(
#if !BUILDFLAG(IS_IOS)
      brave_shields::AdBlockService* adblock_service,
      component_updater::ComponentUpdateService* component_update_service,
#endif  // !BUILDFLAG(IS_IOS)
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  WebcompatReporterService(const WebcompatReporterService&) = delete;
  WebcompatReporterService& operator=(const WebcompatReporterService&) = delete;
  ~WebcompatReporterService() override;

  mojo::PendingRemote<mojom::WebcompatReporterHandler> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::WebcompatReporterHandler> receiver);

  void SubmitWebcompatReport(mojom::ReportInfoPtr report_info) override;

  void SubmitWebcompatReport(Report report_data);

 private:
  friend class WebcompatReporterServiceUnitTest;
  WebcompatReporterService();
  void SetUpWebcompatReporterServiceForTest(
      std::unique_ptr<WebcompatReportUploader> report_uploader
#if !BUILDFLAG(IS_IOS)
      ,component_updater::ComponentUpdateService* component_update_service
#endif  // !BUILDFLAG(IS_IOS)
     );

  void SubmitReportInternal(const Report& report_data);
#if !BUILDFLAG(IS_IOS)
  raw_ptr<component_updater::ComponentUpdateService> component_update_service_;
  raw_ptr<brave_shields::AdBlockService> adblock_service_;
#endif  // !BUILDFLAG(IS_IOS)
  std::unique_ptr<WebcompatReportUploader> report_uploader_;
  mojo::ReceiverSet<mojom::WebcompatReporterHandler> receivers_;
  base::WeakPtrFactory<WebcompatReporterService> weak_factory_{this};
};

}  // namespace webcompat_reporter

#endif  // BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_WEBCOMPAT_REPORTER_SERVICE_H_
