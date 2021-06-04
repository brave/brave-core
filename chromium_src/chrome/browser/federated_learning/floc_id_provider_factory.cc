/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/federated_learning/floc_id_provider_factory.h"

#include "chrome/browser/federated_learning/floc_remote_permission_service_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/privacy_sandbox/privacy_sandbox_settings_factory.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "chrome/browser/sync/user_event_service_factory.h"

#define BuildServiceInstanceFor BuildServiceInstanceFor_ChromiumImpl
#include "../../../../../chrome/browser/federated_learning/floc_id_provider_factory.cc"  // NOLINT
#undef BuildServiceInstanceFor

namespace federated_learning {

// We don't want FLoC:
// https://github.com/brave/brave-browser/issues/14942
KeyedService* FlocIdProviderFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return nullptr;
}

}  // namespace federated_learning
