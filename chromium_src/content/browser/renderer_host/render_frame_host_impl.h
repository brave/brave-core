/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_IMPL_H_

#if defined(OS_ANDROID)
#include "brave/content/browser/mojom/cosmetic_filters_communication.mojom.h"

#define BRAVE_RENDERER_FRAME_HOST_IMPL_H \
  void GetCosmeticFiltersResponder( \
      mojo::PendingReceiver< \
          cf_comm::mojom::CosmeticFiltersCommunication> receiver);
#else
#define BRAVE_RENDERER_FRAME_HOST_IMPL_H
#endif

#include "../../../../../content/browser/renderer_host/render_frame_host_impl.h"  // NOLINT

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_IMPL_H_
