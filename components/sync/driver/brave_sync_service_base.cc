/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/sync/driver/brave_sync_service_base.h"

//#include "brave/components/brave_sync/controller.h"
#include "brave/components/brave_sync/controller_impl.h"


namespace syncer {

BraveSyncServiceBase::BraveSyncServiceBase(
              // std::unique_ptr<SyncClient> sync_client,
              // std::unique_ptr<SigninManagerWrapper> signin,
              // const version_info::Channel& channel,
              // const base::FilePath& base_directory,
              // const std::string& debug_identifier
            )
    // : SyncServiceBase(
    //       std::move(sync_client),
    //       std::move(signin),
    //       channel,
    //       base_directory,
    //       debug_identifier)
          {
LOG(ERROR) << "TAGAB: BraveSyncServiceBase::BraveSyncServiceBase CTOR";
//LOG(ERROR) << base::debug::StackTrace().ToString();
  brave_sync_controller_ = brave_sync::BraveSyncControllerImpl::GetInstance();
  //sync_client->GetPrefService()
}

//BraveSyncServiceBase::~BraveSyncServiceBase() = default;
BraveSyncServiceBase::~BraveSyncServiceBase() {
  LOG(ERROR) << "TAGAB: BraveSyncServiceBase::~BraveSyncServiceBase DTOR";
}

}  // namespace syncer
