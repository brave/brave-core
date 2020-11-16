/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#if defined(OS_ANDROID)
#include "brave/content/browser/cosmetic_filters_communication_impl.h"
#endif
#include "../../../../../content/browser/renderer_host/render_frame_host_impl.cc"

namespace content {

#if defined(OS_ANDROID)
void RenderFrameHostImpl::GetCosmeticFiltersResponder(
      mojo::PendingReceiver<
          cf_comm::mojom::CosmeticFiltersCommunication> receiver) {
  CosmeticFiltersCommunicationImpl::CreateInstance(this, nullptr);
  mojo::MakeSelfOwnedReceiver(
      std::move(cosmetic_filters_communication_impl_),
      std::move(receiver));
}
#endif

}  // namespace content
