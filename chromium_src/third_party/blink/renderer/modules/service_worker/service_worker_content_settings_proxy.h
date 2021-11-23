/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICE_WORKER_SERVICE_WORKER_CONTENT_SETTINGS_PROXY_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICE_WORKER_SERVICE_WORKER_CONTENT_SETTINGS_PROXY_H_

#include "brave/third_party/blink/renderer/brave_farbling_constants.h"

#define BRAVE_SERVICE_WORKER_CONTENT_SETTINGS_PROXY_H           \
  bool AllowFingerprinting(bool enabled_per_settings) override; \
  BraveFarblingLevel GetBraveFarblingLevel() override;

#include "../../../../../../../third_party/blink/renderer/modules/service_worker/service_worker_content_settings_proxy.h"

#undef BRAVE_SERVICE_WORKER_SETTINGS_PROXY_H

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICE_WORKER_SERVICE_WORKER_CONTENT_SETTINGS_PROXY_H_
