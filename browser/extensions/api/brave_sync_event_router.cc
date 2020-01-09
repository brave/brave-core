/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_sync_event_router.h"

#include <memory>
#include <utility>

#include "brave/common/extensions/api/brave_sync.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/extension_event_histogram_value.h"

using extensions::api::brave_sync::RecordAndExistingObject;

namespace extensions {

BraveSyncEventRouter::BraveSyncEventRouter(Profile* profile) :
    event_router_(EventRouter::Get(profile)) {
}

BraveSyncEventRouter::~BraveSyncEventRouter() {}

void BraveSyncEventRouter::GotInitData(
    const brave_sync::Uint8Array& seed,
    const brave_sync::Uint8Array& device_id,
    const extensions::api::brave_sync::Config& config,
    const std::string& device_id_v2) {
  const std::vector<int> arg_seed(seed.begin(), seed.end());
  const std::vector<int> arg_device_id(device_id.begin(), device_id.end());

  std::unique_ptr<base::ListValue> args(
      extensions::api::brave_sync::OnGotInitData::Create(arg_seed,
                                                         arg_device_id, config,
                                                         device_id_v2)
          .release());
  std::unique_ptr<Event> event(
     new Event(extensions::events::FOR_TEST,
       extensions::api::brave_sync::OnGotInitData::kEventName,
       std::move(args)));
  event_router_->BroadcastEvent(std::move(event));
}

void BraveSyncEventRouter::FetchSyncRecords(
    const std::vector<std::string>& category_names,
    const base::Time& startAt,
    const int max_records) {
  std::unique_ptr<base::ListValue> args(
     extensions::api::brave_sync::OnFetchSyncRecords::Create(category_names,
       startAt.ToJsTime(), static_cast<double>(max_records))
       .release());
  std::unique_ptr<Event> event(
     new Event(extensions::events::FOR_TEST,
       extensions::api::brave_sync::OnFetchSyncRecords::kEventName,
       std::move(args)));
  event_router_->BroadcastEvent(std::move(event));
}

void BraveSyncEventRouter::ResolveSyncRecords(
    const std::string& category_name,
    const std::vector<RecordAndExistingObject>& records_and_existing_objects) {
  for (const auto & entry : records_and_existing_objects) {
    DCHECK(!entry.server_record.object_data.empty());
    DCHECK(!entry.local_record ||
        (entry.local_record->object_data == "bookmark" ||
        entry.local_record->object_data == "device" ||
        entry.local_record->object_data == "historySite" ||
        entry.local_record->object_data == "siteSetting"));
  }

  std::unique_ptr<base::ListValue> args(
     extensions::api::brave_sync::OnResolveSyncRecords::Create(
          category_name,
          records_and_existing_objects).release());
  std::unique_ptr<Event> event(
     new Event(extensions::events::FOR_TEST,
       extensions::api::brave_sync::OnResolveSyncRecords::kEventName,
       std::move(args)));

  event_router_->BroadcastEvent(std::move(event));
}

void BraveSyncEventRouter::SendSyncRecords(
    const std::string& category_name,
    const std::vector<api::brave_sync::SyncRecord>& records) {
  std::unique_ptr<base::ListValue> args(
     extensions::api::brave_sync::OnSendSyncRecords::Create(
          category_name,
          records).release());
  std::unique_ptr<Event> event(
     new Event(extensions::events::FOR_TEST,
       extensions::api::brave_sync::OnSendSyncRecords::kEventName,
       std::move(args)));

  event_router_->BroadcastEvent(std::move(event));
}

void BraveSyncEventRouter::SendGetBookmarksBaseOrder(
    const std::string& device_id,
    const std::string& platform) {
  std::unique_ptr<base::ListValue> args(
     extensions::api::brave_sync::OnSendGetBookmarksBaseOrder::Create(
        device_id, platform).release());
  std::unique_ptr<Event> event(
     new Event(extensions::events::FOR_TEST,
       extensions::api::brave_sync::OnSendGetBookmarksBaseOrder::kEventName,
       std::move(args)));

  event_router_->BroadcastEvent(std::move(event));
}

void BraveSyncEventRouter::SendCompact(
    const std::string& category_name) {
  std::unique_ptr<base::ListValue> args(
     extensions::api::brave_sync::SendCompact::Create(
        category_name).release());
  std::unique_ptr<Event> event(
     new Event(extensions::events::FOR_TEST,
       extensions::api::brave_sync::SendCompact::kEventName,
       std::move(args)));

  event_router_->BroadcastEvent(std::move(event));
}

void BraveSyncEventRouter::LoadClient() {
  std::unique_ptr<base::ListValue> args(
     extensions::api::brave_sync::OnLoadClient::Create()
       .release());

  std::unique_ptr<Event> event(
     new Event(extensions::events::FOR_TEST,
       extensions::api::brave_sync::OnLoadClient::kEventName,
       std::move(args)));
  event_router_->BroadcastEvent(std::move(event));
}

}  // namespace extensions
