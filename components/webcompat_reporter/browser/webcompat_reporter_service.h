// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_WEBCOMPAT_REPORTER_SERVICE_H_
#define BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_WEBCOMPAT_REPORTER_SERVICE_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/webcompat_reporter/browser/webcompat_report_uploader.h"
#include "brave/components/webcompat_reporter/common/webcompat_reporter.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace webcompat_reporter {

class WebcompatReporterService : public KeyedService,
                                 public mojom::WebcompatReporterHandler {
 public:
  class Delegate {
   public:
    struct ComponentInfo {
      std::string id;
      std::string name;
      std::string version;
    };
    virtual ~Delegate() = default;

    virtual std::optional<std::vector<std::string>> GetAdblockFilterListNames()
        const = 0;
    virtual std::optional<std::string> GetChannelName() const = 0;
    virtual std::optional<std::vector<ComponentInfo>> GetComponentInfos()
        const = 0;
  };

  WebcompatReporterService(
      std::unique_ptr<Delegate> service_delegate,
      std::unique_ptr<WebcompatReportUploader> report_uploader);
  WebcompatReporterService(const WebcompatReporterService&) = delete;
  WebcompatReporterService& operator=(const WebcompatReporterService&) = delete;
  ~WebcompatReporterService() override;

  mojo::PendingRemote<mojom::WebcompatReporterHandler> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::WebcompatReporterHandler> receiver);

  void SubmitWebcompatReport(mojom::ReportInfoPtr report_info) override;

 private:
  friend class WebcompatReporterServiceUnitTest;

  std::unique_ptr<Delegate> service_delegate_;
  std::unique_ptr<WebcompatReportUploader> report_uploader_;
  mojo::ReceiverSet<mojom::WebcompatReporterHandler> receivers_;
  base::WeakPtrFactory<WebcompatReporterService> weak_factory_{this};
};

}  // namespace webcompat_reporter

#endif  // BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_WEBCOMPAT_REPORTER_SERVICE_H_
