/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_CONTENT_SETTINGS_PROXY_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_CONTENT_SETTINGS_PROXY_IMPL_H_

#define BRAVE_SERVICE_WORKER_SETTINGS_PROXY_IMPL_H                         \
  void AllowFingerprinting(AllowFingerprintingCallback callback) override; \
  void GetBraveFarblingLevel(GetBraveFarblingLevelCallback callback) override;

#include "src/content/browser/service_worker/service_worker_content_settings_proxy_impl.h"

#undef BRAVE_SERVICE_WORKER_SETTINGS_PROXY_IMPL_H

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_CONTENT_SETTINGS_PROXY_IMPL_H_
