/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_BROWSER_CONTEXT_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_BROWSER_CONTEXT_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "brave/content/browser/ephemeral_storage_partition.h"
#include "content/common/content_export.h"
#include "url/gurl.h"

namespace content {

class WebContents;
class EphemeralStoragePartition;
class SessionStorageNamespace;
class StoragePartition;

CONTENT_EXPORT scoped_refptr<content::SessionStorageNamespace>
CreateSessionStorageNamespace(content::StoragePartition* partition,
                              const std::string& namespace_id);

CONTENT_EXPORT std::string GetSessionStorageNamespaceId(WebContents*);

// Convert this URL into an encoded storage domain string, which is used
// to identify a particular storage domain uniquely in a BrowserContext.
CONTENT_EXPORT std::string URLToEphemeralStorageDomain(const GURL& url);

}  // namespace content

#define BRAVE_BROWSER_CONTEXT_H                                     \
 public:                                                            \
  scoped_refptr<EphemeralStoragePartition>                          \
  GetOrCreateEphemeralStoragePartition(std::string storage_domain); \
  EphemeralStoragePartition* GetExistingEphemeralStoragePartition(  \
      const GURL& url);

#include "../../../../../content/public/browser/browser_context.h"

#undef BRAVE_BROWSER_CONTEXT_H

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_BROWSER_CONTEXT_H_
