/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/services/storage/dom_storage/session_storage_impl.h"

#include "../../../../../../components/services/storage/dom_storage/session_storage_impl.cc"

namespace storage {

void SessionStorageImpl::ClearDataInNamespace(
    const std::string& namespace_id,
    ClearDataInNamespaceCallback callback) {
  auto namespace_it = namespaces_.find(namespace_id);
  if (namespace_it != namespaces_.end()) {
    SessionStorageNamespaceImpl* namespace_ptr = namespace_it->second.get();
    namespace_ptr->ClearData(std::move(callback));
  }
}

}  // namespace storage
