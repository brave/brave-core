/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_RENDERER_WORKER_CONTENT_SETTINGS_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_CHROME_RENDERER_WORKER_CONTENT_SETTINGS_CLIENT_H_

#include "third_party/blink/public/platform/web_content_settings_client.h"

class WorkerContentSettingsClient_BraveImpl;

#define EnsureContentSettingsManager()          \
  EnsureContentSettingsManager_NotUsed();       \
  friend WorkerContentSettingsClient_BraveImpl; \
  void EnsureContentSettingsManager()

#define WorkerContentSettingsClient WorkerContentSettingsClient_ChromiumImpl

#include "src/chrome/renderer/worker_content_settings_client.h"  // IWYU pragma: export

#undef EnsureContentSettingsManager
#undef WorkerContentSettingsClient

class WorkerContentSettingsClient_BraveImpl
    : public WorkerContentSettingsClient_ChromiumImpl {
 public:
  explicit WorkerContentSettingsClient_BraveImpl(
      content::RenderFrame* render_frame);
  ~WorkerContentSettingsClient_BraveImpl() override;

  // WebContentSettingsClient:
  std::unique_ptr<blink::WebContentSettingsClient> Clone() override;
  brave_shields::mojom::ShieldsSettingsPtr GetBraveShieldsSettings(
      ContentSettingsType webcompat_settings_type) override;
  blink::WebSecurityOrigin GetEphemeralStorageOriginSync() override;
  bool HasContentSettingsRules() const override;

 private:
  WorkerContentSettingsClient_BraveImpl(
      const WorkerContentSettingsClient_BraveImpl& other);

  brave_shields::mojom::ShieldsSettingsPtr shields_settings_;
};

using WorkerContentSettingsClient = WorkerContentSettingsClient_BraveImpl;

#endif  // BRAVE_CHROMIUM_SRC_CHROME_RENDERER_WORKER_CONTENT_SETTINGS_CLIENT_H_
