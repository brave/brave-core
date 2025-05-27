/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WINDOWS_RECALL_WINDOWS_RECALL_SERVICE_H_
#define BRAVE_COMPONENTS_WINDOWS_RECALL_WINDOWS_RECALL_SERVICE_H_

#include "base/memory/raw_ptr.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefService;

namespace content {
class BrowserContext;
}

namespace windows_recall {

class WindowsRecallService : public KeyedService {
 public:
  explicit WindowsRecallService(PrefService* pref_service);
  WindowsRecallService(const WindowsRecallService&) = delete;
  WindowsRecallService& operator=(WindowsRecallService&) = delete;

  ~WindowsRecallService() override;

  static WindowsRecallService* Get(content::BrowserContext* browser_context);

  bool IsWindowsRecallEnabled() const;

  // Changes pref which will be used after restart.
  void EnableWindowsRecall(bool enable);

 private:
  const bool is_windows_recall_enabled_ = false;
  const raw_ptr<PrefService> pref_service_ = nullptr;
};

}  // namespace windows_recall

#endif  // BRAVE_COMPONENTS_WINDOWS_RECALL_WINDOWS_RECALL_SERVICE_H_
