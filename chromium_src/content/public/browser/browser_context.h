/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_BROWSER_CONTEXT_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_BROWSER_CONTEXT_H_

#define IsOffTheRecord \
  IsTor() const;       \
  virtual bool IsOffTheRecord
#include "../../../../../content/public/browser/browser_context.h"
#undef IsOffTheRecord

#include <string>

#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "content/public/browser/tld_ephemeral_lifetime.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace content {

class WebContents;
class SessionStorageNamespace;
class StoragePartition;

CONTENT_EXPORT scoped_refptr<content::SessionStorageNamespace>
CreateSessionStorageNamespace(
    content::StoragePartition* partition,
    const std::string& namespace_id,
    absl::optional<std::string> clone_from_namespace_id);

CONTENT_EXPORT std::string GetSessionStorageNamespaceId(WebContents*);

}  // namespace content

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_BROWSER_CONTEXT_H_
