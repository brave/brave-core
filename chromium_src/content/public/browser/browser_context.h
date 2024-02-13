/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_BROWSER_CONTEXT_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_BROWSER_CONTEXT_H_

#define IsOffTheRecord \
  IsTor() const;       \
  virtual bool IsOffTheRecord
#include <optional>

#include "src/content/public/browser/browser_context.h"  // IWYU pragma: export
#undef IsOffTheRecord

#include <string>

#include "components/services/storage/public/mojom/blob_storage_context.mojom.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace content {

class WebContents;
class SessionStorageNamespace;
class StoragePartition;

CONTENT_EXPORT mojo::PendingRemote<storage::mojom::BlobStorageContext>
GetRemoteBlobStorageContextFor(BrowserContext* browser_context);

CONTENT_EXPORT scoped_refptr<content::SessionStorageNamespace>
CreateSessionStorageNamespace(
    content::StoragePartition* partition,
    const std::string& namespace_id,
    std::optional<std::string> clone_from_namespace_id);

CONTENT_EXPORT std::string GetSessionStorageNamespaceId(WebContents*);

}  // namespace content

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_BROWSER_CONTEXT_H_
