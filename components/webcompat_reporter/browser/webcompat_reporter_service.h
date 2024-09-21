// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_WEBCOMPAT_REPORTER_SERVICE_H_
#define BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_WEBCOMPAT_REPORTER_SERVICE_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/webcompat_reporter/common/webcompat_reporter.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace webcompat_reporter {

// This class is not thread-safe and should have single owner
class WebcompatReporterService : public KeyedService,
                          public mojom::WebcompatReporterHandler {
 public:
  explicit WebcompatReporterService(component_updater::ComponentUpdateService* component_update_service);
  ~WebcompatReporterService() override;

   mojo::PendingRemote<mojom::WebcompatReporterHandler> MakeRemote();
   void Bind(mojo::PendingReceiver<mojom::WebcompatReporterHandler> receiver);

   void GetAdblockComponentInfo(GetAdblockComponentInfoCallback callback) override;

 private:
  raw_ptr<component_updater::ComponentUpdateService> component_update_service_;
  mojo::ReceiverSet<mojom::WebcompatReporterHandler> receivers_;
  base::WeakPtrFactory<WebcompatReporterService> weak_factory_{this};

  WebcompatReporterService(const WebcompatReporterService&) = delete;
  WebcompatReporterService& operator=(const WebcompatReporterService&) = delete;
};

}  // namespace webcompat_reporter

#endif  // BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_WEBCOMPAT_REPORTER_SERVICE_H_
