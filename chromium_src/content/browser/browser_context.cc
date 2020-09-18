/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "content/public/browser/browser_context.h"

#include <string>

#include "base/memory/ref_counted.h"
#include "content/browser/dom_storage/dom_storage_context_wrapper.h"
#include "content/browser/dom_storage/session_storage_namespace_impl.h"
#include "content/public/browser/session_storage_namespace.h"
#include "content/public/browser/storage_partition.h"

namespace content {

scoped_refptr<content::SessionStorageNamespace> CreateSessionStorageNamespace(
    content::StoragePartition* partition,
    const std::string& namespace_id) {
  content::DOMStorageContextWrapper* context_wrapper =
      static_cast<content::DOMStorageContextWrapper*>(
            partition->GetDOMStorageContext());

  return content::SessionStorageNamespaceImpl::Create(context_wrapper,
                                                      namespace_id);
}

}  // namespace content

#include "../../../../content/browser/browser_context.cc"
