/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_DELEGATE_H_
#define BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_DELEGATE_H_

#include <string>

#include "base/functional/callback.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_types.h"

namespace ephemeral_storage {

// Delegate performs cleanup for all required parts (chrome, content, etc.).
class EphemeralStorageServiceDelegate {
 public:
  virtual ~EphemeralStorageServiceDelegate() = default;

  // Cleanups ephemeral storages (local storage, cookies).
  virtual void CleanupTLDEphemeralArea(const TLDEphemeralAreaKey& key) = 0;
  // Cleanups non-ephemeral first party storage areas (cache, dom storage).
  virtual void CleanupFirstPartyStorageArea(
      const std::string& registerable_domain) = 0;
  // Registers a callback to be called when the first window is opened.
  virtual void RegisterFirstWindowOpenedCallback(
      base::OnceClosure callback) = 0;
};

}  // namespace ephemeral_storage

#endif  // BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_DELEGATE_H_
