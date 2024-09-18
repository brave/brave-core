/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/content/browser/worker_host/shared_worker_host.cc"

namespace content {

void SharedWorkerHost::GetBraveShieldsSettings(
    const GURL& url,
    base::OnceCallback<void(brave_shields::mojom::ShieldsSettingsPtr)>
        callback) {
  std::move(callback).Run(
      GetContentClient()->browser()->WorkerGetBraveShieldSettings(
          url, GetProcessHost()->GetBrowserContext()));
}

}  // namespace content
