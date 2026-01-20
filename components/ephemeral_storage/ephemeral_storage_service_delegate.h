/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_DELEGATE_H_
#define BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_DELEGATE_H_

#include <string_view>

#include "base/containers/flat_set.h"
#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_types.h"
#include "url/gurl.h"

namespace ephemeral_storage {

// Delegate performs cleanup for all required parts (chrome, content, etc.).
class EphemeralStorageServiceDelegate {
 public:
  virtual ~EphemeralStorageServiceDelegate() = default;

  // Cleanups ephemeral storages (local storage, cookies).
  virtual void CleanupTLDEphemeralArea(const TLDEphemeralAreaKey& key) = 0;
  // Cleanups non-ephemeral first party storage areas (cache, dom storage).
  virtual void CleanupFirstPartyStorageArea(const TLDEphemeralAreaKey& key) = 0;
  // Registers a callback to be called when the first window is opened.
  virtual void RegisterFirstWindowOpenedCallback(
      base::OnceClosure callback) = 0;
  // Registers a callback to be called when the browser started and becomes
  // active.
  virtual void RegisterOnBecomeActiveCallback(
      base::OnceCallback<void(const base::flat_set<TLDEphemeralAreaKey>)>
          callback) = 0;
  // Finds all tabs related to the ephemeral_domains list, prepares them for
  // first party storage cleanup, and closes them.
  virtual void PrepareTabsForFirstPartyStorageCleanup(
      const std::vector<std::string>& ephemeral_domains) = 0;
  virtual bool IsShieldsDisabledOnAnyHostMatchingDomainOf(
      const GURL& url) const = 0;
#if BUILDFLAG(IS_ANDROID)
  // Triggers notification of current app state on Android. We need to call it
  // at the beginning of the TLD ephemeral lifetime.
  virtual void TriggerCurrentAppStateNotification() = 0;
#endif
};

}  // namespace ephemeral_storage

#endif  // BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_DELEGATE_H_
