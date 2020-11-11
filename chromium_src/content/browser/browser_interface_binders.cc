/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define PopulateBinderMap PopulateBinderMap_ChromiumImpl
#include "../../../../content/browser/browser_interface_binders.cc"
#undef PopulateBinderMap

namespace content {
namespace internal {

void PopulateBinderMap(RenderFrameHostImpl* host, mojo::BinderMap* map) {
  PopulateBinderMap_ChromiumImpl(host, map);
#if defined(OS_ANDROID)
  // Register the handler for cosmetic filters responder.
  map->Add<cf_comm::mojom::CosmeticFiltersCommunication>(base::BindRepeating(
      &RenderFrameHostImpl::GetCosmeticFiltersResponder,
          base::Unretained(host)));
#endif
}

void PopulateBinderMap(DedicatedWorkerHost* host, mojo::BinderMap* map) {
  PopulateBinderMap_ChromiumImpl(host, map);
}

void PopulateBinderMap(SharedWorkerHost* host, mojo::BinderMap* map) {
  PopulateBinderMap_ChromiumImpl(host, map);
}

void PopulateBinderMap(ServiceWorkerHost* host, mojo::BinderMap* map) {
  PopulateBinderMap_ChromiumImpl(host, map);
}

}  // namespace internal
}  // namespace content
