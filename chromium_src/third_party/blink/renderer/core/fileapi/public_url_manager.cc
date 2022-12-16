/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "net/base/features.h"
#include "third_party/blink/public/mojom/blob/blob_registry.mojom-blink.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/public/platform/web_security_origin.h"

namespace blink {
namespace {

WebSecurityOrigin GetEphemeralOrOriginalSecurityOrigin(
    ExecutionContext* context,
    const SecurityOrigin* origin) {
  if (base::FeatureList::IsEnabled(net::features::kBravePartitionBlobStorage)) {
    if (WebContentSettingsClient* settings =
            brave::GetContentSettingsClientFor(context)) {
      WebSecurityOrigin ephemeral_storage_origin =
          settings->GetEphemeralStorageOriginSync();
      if (!ephemeral_storage_origin.IsNull()) {
        return ephemeral_storage_origin;
      }
    }
  }

  return base::WrapRefCounted(origin);
}

}  // namespace
}  // namespace blink

#define URLStoreForOrigin(ORIGIN, URL_STORE)                                 \
  URLStoreForOrigin(                                                         \
      GetEphemeralOrOriginalSecurityOrigin(execution_context, ORIGIN).Get(), \
      URL_STORE);

#include "src/third_party/blink/renderer/core/fileapi/public_url_manager.cc"

#undef URLStoreForOrigin
