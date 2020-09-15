/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SERVICES_STORAGE_DOM_STORAGE_SESSION_STORAGE_NAMESPACE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SERVICES_STORAGE_DOM_STORAGE_SESSION_STORAGE_NAMESPACE_IMPL_H_

// Brave-specific: allows embedder to request deletion of an in-memory
// StoragePartition.
#define BRAVE_SESSION_STORAGE_NAMESPACE_IMPL_H \
  void ClearData(base::OnceClosure callback);

#include "../../../../../../components/services/storage/dom_storage/session_storage_namespace_impl.h"

#undef BRAVE_SESSION_STORAGE_NAMESPACE_IMPL_H

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SERVICES_STORAGE_DOM_STORAGE_SESSION_STORAGE_NAMESPACE_IMPL_H_
