/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/api/brave_sync_event_router.h"
#include "brave/common/extensions/api/brave_sync.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/extension_event_histogram_value.h"
#include "content/public/browser/browser_thread.h"

namespace extensions {

BraveSyncEventRouter::BraveSyncEventRouter(Profile* profile) : profile_(profile) {
  ;
}

BraveSyncEventRouter::~BraveSyncEventRouter() {
  ;
}

void BraveSyncEventRouter::BrowserToBackgroundPage(const std::string &arg1) {
   if (!profile_) {
     LOG(ERROR) << "TAGAB BraveSyncEventRouter::BrowserToBackgroundPage profile is not set";
     return;
   }

   EventRouter* event_router = EventRouter::Get(profile_);

   if (event_router) {
    std::unique_ptr<base::ListValue> args(
        extensions::api::brave_sync::OnBrowserToBackgroundPage::Create(arg1)
          .release());
    std::unique_ptr<Event> event(
        new Event(extensions::events::FOR_TEST,
          extensions::api::brave_sync::OnBrowserToBackgroundPage::kEventName,
          std::move(args)));
    event_router->BroadcastEvent(std::move(event));
  }
}

void BraveSyncEventRouter::GotInitData(
  const brave_sync::Uint8Array &seed,
  const brave_sync::Uint8Array &device_id,
  const extensions::api::brave_sync::Config &config
) {
  if (!profile_) {
    LOG(ERROR) << "TAGAB BraveSyncEventRouter::GotInitData profile is not set";
    return;
  }

  EventRouter* event_router = EventRouter::Get(profile_);

  if (event_router) {
    const std::vector<int> arg_seed(seed.begin(), seed.end());
    const std::vector<int> arg_device_id(device_id.begin(), device_id.end());

    std::unique_ptr<base::ListValue> args(
       extensions::api::brave_sync::OnGotInitData::Create(arg_seed, arg_device_id, config)
         .release());
    std::unique_ptr<Event> event(
       new Event(extensions::events::FOR_TEST,
         extensions::api::brave_sync::OnGotInitData::kEventName,
         std::move(args)));
    event_router->BroadcastEvent(std::move(event));
  }
}

void BraveSyncEventRouter::FetchSyncRecords(
    const std::vector<std::string> &category_names,
    const base::Time &startAt,
    const int &max_records) {
  if (!profile_) {
    LOG(ERROR) << "TAGAB BraveSyncEventRouter::FetchSyncRecords profile is not set";
    return;
  }

  EventRouter* event_router = EventRouter::Get(profile_);

  if (!event_router) {
    return;
  }

  std::unique_ptr<base::ListValue> args(
     extensions::api::brave_sync::OnFetchSyncRecords::Create(category_names,
       startAt.ToJsTime(), static_cast<double>(max_records))
       .release());
  std::unique_ptr<Event> event(
     new Event(extensions::events::FOR_TEST,
       extensions::api::brave_sync::OnFetchSyncRecords::kEventName,
       std::move(args)));
  event_router->BroadcastEvent(std::move(event));
  ;
}

void BraveSyncEventRouter::ResolveSyncRecords(const std::string &category_name,
  const std::vector<extensions::api::brave_sync::RecordAndExistingObject>& records_and_existing_objects) {
  if (!profile_) {
    LOG(ERROR) << "TAGAB BraveSyncEventRouter::ResolveSyncRecords profile is not set";
    return;
  }

  EventRouter* event_router = EventRouter::Get(profile_);

  if (!event_router) {
    return;
  }

  std::unique_ptr<base::ListValue> args(
     extensions::api::brave_sync::OnResolveSyncRecords::Create(category_name, records_and_existing_objects)
       .release());
  std::unique_ptr<Event> event(
     new Event(extensions::events::FOR_TEST,
       extensions::api::brave_sync::OnResolveSyncRecords::kEventName,
       std::move(args)));

  LOG(ERROR) << "TAGAB BraveSyncEventRouter::ResolveSyncRecords: will post to in BrowserThread::UI";
  content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::UI)->PostTask(
    FROM_HERE, base::Bind(&EventRouter::BroadcastEvent,
         base::Unretained(event_router), base::Passed(std::move(event))));
}

void BraveSyncEventRouter::SendSyncRecords(const std::string &category_name,
  const std::vector<api::brave_sync::SyncRecord2>& records) {
  if (!profile_) {
    LOG(ERROR) << "profile is not set";
    return;
  }

  EventRouter* event_router = EventRouter::Get(profile_);

  if (!event_router) {
    return;
  }

  std::unique_ptr<base::ListValue> args(
     extensions::api::brave_sync::OnSendSyncRecords::Create(category_name, records)
       .release());
  std::unique_ptr<Event> event(
     new Event(extensions::events::FOR_TEST,
       extensions::api::brave_sync::OnSendSyncRecords::kEventName,
       std::move(args)));

  LOG(ERROR) << "TAGAB BraveSyncEventRouter::SendSyncRecords: will post to in BrowserThread::UI";
  content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::UI)->PostTask(
    FROM_HERE, base::Bind(&EventRouter::BroadcastEvent,
         base::Unretained(event_router), base::Passed(std::move(event))));

}

void BraveSyncEventRouter::NeedSyncWords(const std::string &seed) {
  if (!profile_) {
    LOG(ERROR) << "profile is not set";
    return;
  }

  EventRouter* event_router = EventRouter::Get(profile_);

  if (!event_router) {
    return;
  }

  std::unique_ptr<base::ListValue> args(
     extensions::api::brave_sync::OnNeedSyncWords::Create(seed)
       .release());
  std::unique_ptr<Event> event(
     new Event(extensions::events::FOR_TEST,
       extensions::api::brave_sync::OnNeedSyncWords::kEventName,
       std::move(args)));
  event_router->BroadcastEvent(std::move(event));
}

void BraveSyncEventRouter::NeedBytesFromSyncWords(const std::string &words) {
  if (!profile_) {
    LOG(ERROR) << "profile is not set";
    return;
  }

  EventRouter* event_router = EventRouter::Get(profile_);

  if (!event_router) {
    return;
  }

  std::unique_ptr<base::ListValue> args(
     extensions::api::brave_sync::OnNeedBytesFromSyncWords::Create(words)
       .release());
  std::unique_ptr<Event> event(
     new Event(extensions::events::FOR_TEST,
       extensions::api::brave_sync::OnNeedBytesFromSyncWords::kEventName,
       std::move(args)));
  event_router->BroadcastEvent(std::move(event));
}

void BraveSyncEventRouter::LoadClient() {
  if (!profile_) {
    LOG(ERROR) << "profile is not set";
    return;
  }

  EventRouter* event_router = EventRouter::Get(profile_);

  if (!event_router) {
    return;
  }

  std::unique_ptr<base::ListValue> args(
     extensions::api::brave_sync::OnLoadClient::Create()
       .release());

  std::unique_ptr<Event> event(
     new Event(extensions::events::FOR_TEST,
       extensions::api::brave_sync::OnLoadClient::kEventName,
       std::move(args)));
  event_router->BroadcastEvent(std::move(event));
}

} // namespace extensions
