/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_BROWSER_CONTEXT_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_BROWSER_CONTEXT_H_

#include "../../../../../content/public/browser/browser_context.h"

#include <string>

#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"

namespace content {

class SessionStorageNamespace;
class StoragePartition;

CONTENT_EXPORT scoped_refptr<content::SessionStorageNamespace> CreateSessionStorageNamespace(
    content::StoragePartition* partition,
    const std::string& namespace_id);

}  // namespace content

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_BROWSER_CONTEXT_H_
