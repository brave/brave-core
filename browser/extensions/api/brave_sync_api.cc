/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_sync_api.h"

#include <memory>
#include <utility>
#include <vector>

#include "brave/common/extensions/api/brave_sync.h"
#include "brave/components/brave_sync/brave_sync_service.h"
#include "brave/components/brave_sync/client/brave_sync_client.h"
#include "brave/components/brave_sync/client/client_ext_impl_data.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "brave/components/brave_sync/public/brave_profile_sync_service.h"
#include "brave/components/brave_sync/values_conv.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"

using brave_sync::BraveProfileSyncService;
using brave_sync::BraveSyncService;
using content::BrowserContext;

namespace extensions {
namespace api {

namespace {

BraveProfileSyncService* GetProfileSyncService(
    BrowserContext* browser_context) {
  return static_cast<BraveProfileSyncService*>(
      ProfileSyncServiceFactory::GetAsProfileSyncServiceForProfile(
          Profile::FromBrowserContext(browser_context)));
}

BraveSyncService* GetSyncService(BrowserContext* browser_context) {
  return GetProfileSyncService(browser_context)->GetSyncService();
}

}  // namespace
ExtensionFunction::ResponseAction BraveSyncGetInitDataFunction::Run() {
  std::unique_ptr<brave_sync::GetInitData::Params> params(
      brave_sync::GetInitData::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  BraveSyncService* sync_service = GetSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetBraveSyncClient()->sync_message_handler()->OnGetInitData(
      params->sync_version);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncSyncSetupErrorFunction::Run() {
  std::unique_ptr<brave_sync::SyncSetupError::Params> params(
      brave_sync::SyncSetupError::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  BraveSyncService* sync_service = GetSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetBraveSyncClient()->sync_message_handler()->OnSyncSetupError(
      params->error);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncSyncDebugFunction::Run() {
  std::unique_ptr<brave_sync::SyncDebug::Params> params(
      brave_sync::SyncDebug::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  BraveSyncService* sync_service = GetSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetBraveSyncClient()->sync_message_handler()->OnSyncDebug(
      params->message);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncSaveInitDataFunction::Run() {
  std::unique_ptr<brave_sync::SaveInitData::Params> params(
      brave_sync::SaveInitData::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  BraveSyncService* sync_service = GetSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetBraveSyncClient()->sync_message_handler()->OnSaveInitData(
      params->seed ? *params->seed : std::vector<uint8_t>(),
      params->device_id ? *params->device_id : std::vector<uint8_t>(),
      params->device_id_v2 ? *params->device_id_v2 : std::string());

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncSyncReadyFunction::Run() {
  BraveSyncService* sync_service = GetSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetBraveSyncClient()->sync_message_handler()->OnSyncReady();

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncGetExistingObjectsFunction::Run() {
  std::unique_ptr<brave_sync::GetExistingObjects::Params> params(
      brave_sync::GetExistingObjects::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto records = std::make_unique<std::vector<::brave_sync::SyncRecordPtr>>();
  ::brave_sync::ConvertSyncRecords(params->records, records.get());

  BraveSyncService* sync_service = GetSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetBraveSyncClient()
      ->sync_message_handler()
      ->OnGetExistingObjects(
          params->category_name, std::move(records),
          base::Time::FromJsTime(params->last_record_timestamp),
          params->is_truncated);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncResolvedSyncRecordsFunction::Run() {
  std::unique_ptr<brave_sync::ResolvedSyncRecords::Params> params(
      brave_sync::ResolvedSyncRecords::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto records = std::make_unique<std::vector<::brave_sync::SyncRecordPtr>>();
  ::brave_sync::ConvertSyncRecords(params->records, records.get());

  BraveSyncService* sync_service = GetSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetBraveSyncClient()
      ->sync_message_handler()
      ->OnResolvedSyncRecords(params->category_name, std::move(records));

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
BraveSyncSaveBookmarksBaseOrderFunction::Run() {
  std::unique_ptr<brave_sync::SaveBookmarksBaseOrder::Params> params(
      brave_sync::SaveBookmarksBaseOrder::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  BraveSyncService* sync_service = GetSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetBraveSyncClient()
      ->sync_message_handler()
      ->OnSaveBookmarksBaseOrder(params->order);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
BraveSyncOnCompactCompleteFunction::Run() {
  std::unique_ptr<brave_sync::OnCompactComplete::Params> params(
      brave_sync::OnCompactComplete::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  BraveSyncService* sync_service = GetSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetBraveSyncClient()
      ->sync_message_handler()
      ->OnCompactComplete(params->category_name);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
BraveSyncOnRecordsSentFunction::Run() {
  std::unique_ptr<brave_sync::OnRecordsSent::Params> params(
      brave_sync::OnRecordsSent::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto records = std::make_unique<std::vector<::brave_sync::SyncRecordPtr>>();
  ::brave_sync::ConvertSyncRecords(params->records, records.get());

  BraveSyncService* sync_service = GetSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetBraveSyncClient()
      ->sync_message_handler()
      ->OnRecordsSent(params->category_name, std::move(records));

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncExtensionInitializedFunction::Run() {
  // Also inform sync client extension started
  BraveSyncService* sync_service = GetSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetBraveSyncClient()->OnExtensionInitialized();

  return RespondNow(NoArguments());
}

}  // namespace api
}  // namespace extensions
