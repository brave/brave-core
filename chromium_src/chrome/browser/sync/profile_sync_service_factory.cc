/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/sync/driver/profile_sync_service.h"

class Profile;

namespace {
std::unique_ptr<syncer::ProfileSyncService> BraveBuildServiceInstanceFor(
    Profile* profile,
    syncer::ProfileSyncService::InitParams init_params);
}  // namespace

#include "../../../../../chrome/browser/sync/profile_sync_service_factory.cc"  // NOLINT

#include "brave/components/brave_sync/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_SYNC)
#include "brave/components/brave_sync/brave_profile_sync_service_impl.h"
using brave_sync::BraveProfileSyncServiceImpl;
#else
namespace {
class BraveProfileSyncService : public syncer::ProfileSyncService {
 public:
  explicit BraveProfileSyncService(InitParams init_params) :
    syncer::ProfileSyncService(std::move(init_params)) {}
  ~BraveProfileSyncService() override {}

  // syncer::SyncService implementation
  CoreAccountInfo GetAuthenticatedAccountInfo() const override {
    AccountInfo account_info;
    account_info.account_id = "dummy_account_id";
    return std::move(account_info);
  }
};
}  // namespace
#endif

namespace {

std::unique_ptr<syncer::ProfileSyncService> BraveBuildServiceInstanceFor(
    Profile* profile,
    syncer::ProfileSyncService::InitParams init_params) {
#if BUILDFLAG(ENABLE_BRAVE_SYNC)
  return std::make_unique<BraveProfileSyncServiceImpl>(profile,
                                                   std::move(init_params));
#else
  return std::make_unique<BraveProfileSyncService>(std::move(init_params));
#endif
}

}  // namespace
