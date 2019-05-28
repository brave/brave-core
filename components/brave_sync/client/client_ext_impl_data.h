/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_CLIENT_EXT_IMPL_DATA_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_CLIENT_EXT_IMPL_DATA_H_

#include <memory>
#include <vector>

#include "brave/components/brave_sync/jslib_messages_fwd.h"

namespace extensions {
namespace api {
namespace brave_sync {
struct Config;
struct SyncRecord;
struct RecordAndExistingObject;
}
}
}

namespace brave_sync {
namespace client_data {
class Config;
}
}

namespace brave_sync {

using extensions::api::brave_sync::SyncRecord;
using extensions::api::brave_sync::RecordAndExistingObject;

void ConvertConfig(const brave_sync::client_data::Config &config,
                   extensions::api::brave_sync::Config* config_extension);

void ConvertSyncRecords(const std::vector<SyncRecord> &records_extension,
                        std::vector<brave_sync::SyncRecordPtr>* records);

void ConvertResolvedPairs(
    const SyncRecordAndExistingList &records_and_existing_objects,
    std::vector<RecordAndExistingObject>* records_and_existing_objects_ext);

void ConvertSyncRecordsFromLibToExt(
    const std::vector<brave_sync::SyncRecordPtr> &records,
    std::vector<SyncRecord>* records_extension);

}   // namespace brave_sync

#endif    // BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_CLIENT_EXT_IMPL_DATA_H_
