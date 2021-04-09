/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_search/common/brave_search.mojom.h"
#include "content/browser/process_internals/process_internals.mojom.h"
#include "content/browser/service_worker/service_worker_host.h"
#include "content/public/common/content_client.h"

namespace content {
namespace internal {

void BindBraveSearchHost(
    mojo::PendingReceiver<brave_search::mojom::BraveSearchFallback> receiver) {
  GetContentClient()->browser()->BindBraveSearchHost(std::move(receiver));
}

void PopulateServiceWorkerBindersBrave(ServiceWorkerHost* host,
                                       mojo::BinderMap* map) {
  map->Add<brave_search::mojom::BraveSearchFallback>(
      base::BindRepeating(&BindBraveSearchHost));
}

}  // namespace internal
}  // namespace content

#include "../../../../content/browser/browser_interface_binders.cc"
