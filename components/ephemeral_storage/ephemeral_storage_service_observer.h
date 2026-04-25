/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_OBSERVER_H_
#define BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_OBSERVER_H_

#include "base/observer_list_types.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_types.h"

namespace ephemeral_storage {

// Observer to monitor Ephemeral Storage events.
class EphemeralStorageServiceObserver : public base::CheckedObserver {
 public:
  virtual void OnCleanupTLDEphemeralArea(const TLDEphemeralAreaKey& key) {}
};

}  // namespace ephemeral_storage

#endif  // BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_OBSERVER_H_
