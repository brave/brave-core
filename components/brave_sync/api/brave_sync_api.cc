/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/api/brave_sync_api.h"

#include "brave/common/extensions/api/brave_sync.h"
#include "brave/components/brave_sync/client/brave_sync_client.h"
#include "brave/components/brave_sync/brave_sync_service.h"
#include "brave/components/brave_sync/brave_sync_service_factory.h"
#include "brave/components/brave_sync/client/client_ext_impl_data.h"
#include "brave/components/brave_sync/values_conv.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "chrome/browser/profiles/profile.h"

using ::brave_sync::BraveSyncService;
using ::brave_sync::BraveSyncServiceFactory;
using content::BrowserContext;

namespace extensions {
namespace api {

namespace {

BraveSyncService* GetBraveSyncService(BrowserContext* browser_context) {
  return BraveSyncServiceFactory::GetForProfile(
      Profile::FromBrowserContext(browser_context));
}

}  // namespace
ExtensionFunction::ResponseAction BraveSyncGetInitDataFunction::Run() {
  std::unique_ptr<brave_sync::GetInitData::Params> params(
      brave_sync::GetInitData::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  BraveSyncService* sync_service = GetBraveSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetSyncClient()->sync_message_handler()->OnGetInitData(
      params->sync_version);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncSyncSetupErrorFunction::Run() {
  std::unique_ptr<brave_sync::SyncSetupError::Params> params(
      brave_sync::SyncSetupError::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  BraveSyncService* sync_service = GetBraveSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetSyncClient()->sync_message_handler()->OnSyncSetupError(
      params->error);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncSyncDebugFunction::Run() {
  std::unique_ptr<brave_sync::SyncDebug::Params> params(
      brave_sync::SyncDebug::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  BraveSyncService* sync_service = GetBraveSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetSyncClient()->sync_message_handler()->OnSyncDebug(
      params->message);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncSaveInitDataFunction::Run() {
  std::unique_ptr<brave_sync::SaveInitData::Params> params(
      brave_sync::SaveInitData::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  BraveSyncService* sync_service = GetBraveSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetSyncClient()->sync_message_handler()->OnSaveInitData(
      ::brave_sync::Uint8ArrayFromUnsignedCharVec(
          params->seed ? *params->seed : std::vector<uint8_t>()),
      ::brave_sync::Uint8ArrayFromUnsignedCharVec(
          params->device_id ? *params->device_id : std::vector<uint8_t>() )
  );

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncSyncReadyFunction::Run() {
  BraveSyncService* sync_service = GetBraveSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetSyncClient()->sync_message_handler()->OnSyncReady();

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncGetExistingObjectsFunction::Run() {
  std::unique_ptr<brave_sync::GetExistingObjects::Params> params(
      brave_sync::GetExistingObjects::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto records = std::make_unique<std::vector<::brave_sync::SyncRecordPtr>>();
  ::brave_sync::ConvertSyncRecords(params->records, *records.get());

  BraveSyncService* sync_service = GetBraveSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetSyncClient()->sync_message_handler()->OnGetExistingObjects(
    params->category_name,
    std::move(records),
    base::Time::FromJsTime(params->last_record_timestamp),
    params->is_truncated);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncResolvedSyncRecordsFunction::Run() {
  std::unique_ptr<brave_sync::ResolvedSyncRecords::Params> params(
      brave_sync::ResolvedSyncRecords::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto records = std::make_unique<std::vector<::brave_sync::SyncRecordPtr>>();
  ::brave_sync::ConvertSyncRecords(params->records, *records.get());

  BraveSyncService* sync_service = GetBraveSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetSyncClient()->sync_message_handler()->OnResolvedSyncRecords(
    params->category_name,
    std::move(records));

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncSaveBookmarksBaseOrderFunction::Run() {
  std::unique_ptr<brave_sync::SaveBookmarksBaseOrder::Params> params(
      brave_sync::SaveBookmarksBaseOrder::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  BraveSyncService* sync_service = GetBraveSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetSyncClient()->sync_message_handler()->OnSaveBookmarksBaseOrder(
      params->order);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncSaveBookmarkOrderFunction::Run() {
  std::unique_ptr<brave_sync::SaveBookmarkOrder::Params> params(
      brave_sync::SaveBookmarkOrder::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  BraveSyncService* sync_service = GetBraveSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetSyncClient()->sync_message_handler()->OnSaveBookmarkOrder(
      params->order,
      params->prev_order,
      params->next_order,
      params->parent_order);

  // we have the order response so the client can continue syncing
  sync_service->GetSyncClient()->sync_message_handler()->
      BackgroundSyncStarted(false);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncSyncWordsPreparedFunction::Run() {
  std::unique_ptr<brave_sync::SyncWordsPrepared::Params> params(
      brave_sync::SyncWordsPrepared::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  BraveSyncService* sync_service = GetBraveSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetSyncClient()->sync_message_handler()->OnSyncWordsPrepared(params->words);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncExtensionInitializedFunction::Run() {
  // Also inform sync client extension started
  BraveSyncService* sync_service = GetBraveSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetSyncClient()->OnExtensionInitialized();

  return RespondNow(NoArguments());
}

}  // namespace api
}  // namespace extensions
