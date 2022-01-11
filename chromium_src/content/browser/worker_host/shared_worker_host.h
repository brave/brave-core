/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_WORKER_HOST_SHARED_WORKER_HOST_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_WORKER_HOST_SHARED_WORKER_HOST_H_

#define CreateNetworkFactoryParamsForSubresources                         \
  UnusedFunction();                                                       \
  void AllowFingerprinting(const GURL& url,                               \
                           base::OnceCallback<void(bool)> callback);      \
  void GetBraveFarblingLevel(const GURL& url,                             \
                             base::OnceCallback<void(uint8_t)> callback); \
  network::mojom::URLLoaderFactoryParamsPtr                               \
      CreateNetworkFactoryParamsForSubresources

#include "src/content/browser/worker_host/shared_worker_host.h"

#undef CreateNetworkFactoryParamsForSubresources

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_WORKER_HOST_SHARED_WORKER_HOST_H_
