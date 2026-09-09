/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/content/browser/workspace_service_worker.h"

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/storage_partition.h"
#include "third_party/blink/public/common/storage_key/storage_key.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_registration_options.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace ai_chat {

namespace {

void OnRegistered(blink::ServiceWorkerStatusCode status) {
  if (status != blink::ServiceWorkerStatusCode::kOk) {
    LOG(ERROR) << "Leo workspace service worker registration failed: "
               << blink::ServiceWorkerStatusToString(status);
  }
}

}  // namespace

void RegisterWorkspaceServiceWorker(content::BrowserContext* browser_context) {
  CHECK(browser_context);

  const GURL scope(kAIChatLeoWorkspaceUIURL);
  blink::mojom::ServiceWorkerRegistrationOptions options;
  options.scope = scope;
  // The script is shipped as a resource of the same data source that serves the
  // workspace page, and is not bundled, so it needs no module resolution.
  options.type = blink::mojom::ScriptType::kClassic;
  // The script only ever changes with the browser, and there is no network to
  // revalidate against.
  options.update_via_cache =
      blink::mojom::ServiceWorkerUpdateViaCache::kImports;

  content::ServiceWorkerContext* service_worker_context =
      browser_context->GetDefaultStoragePartition()->GetServiceWorkerContext();
  service_worker_context->RegisterServiceWorker(
      GURL(base::StrCat(
          {kAIChatLeoWorkspaceUIURL, kAIChatLeoWorkspaceServiceWorkerScript})),
      blink::StorageKey::CreateFirstParty(url::Origin::Create(scope)), options,
      content::GlobalRenderFrameHostId(), base::BindOnce(&OnRegistered));
}

}  // namespace ai_chat
