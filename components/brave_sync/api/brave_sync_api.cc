/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/api/brave_sync_api.h"

#include "brave/common/extensions/api/brave_sync.h"
#include "brave/components/brave_sync/client/brave_sync_client.h"
#include "brave/components/brave_sync/client/client_ext_impl_data.h"
#include "brave/components/brave_sync/client/brave_sync_client_factory.h"
#include "brave/components/brave_sync/values_conv.h"
#include "brave/components/brave_sync/jslib_messages.h"

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction BraveSyncBackgroundPageToBrowserFunction::Run() {
  LOG(ERROR) << "TAGAB BraveSyncBackgroundPageToBrowserFunction::Run";
  std::unique_ptr<brave_sync::BackgroundPageToBrowser::Params> params(
      brave_sync::BackgroundPageToBrowser::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  LOG(ERROR) << "TAGAB BraveSyncBackgroundPageToBrowserFunction::Run params->message=" << params->message;

  // if (error) {
  //   return RespondNow(Error(error));
  // }

  auto result = std::make_unique<base::Value>(43);
  return RespondNow(OneArgument(std::move(result)));
}

ExtensionFunction::ResponseAction BraveSyncGetInitDataFunction::Run() {
  LOG(ERROR) << "TAGAB BraveSyncGetInitDataFunction::Run";
  std::unique_ptr<brave_sync::GetInitData::Params> params(
      brave_sync::GetInitData::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  LOG(ERROR) << "TAGAB BraveSyncGetInitDataFunction::Run params->sync_version=" << params->sync_version;
  // if (error) {
  //   return RespondNow(Error(error));
  // }

  ::brave_sync::BraveSyncClient* sync_client = ::brave_sync::BraveSyncClientFactory::GetForBrowserContext(browser_context());
  sync_client->GetSyncToBrowserHandler()->OnGetInitData(params->sync_version);

  auto result = std::make_unique<base::Value>(43);
  return RespondNow(OneArgument(std::move(result)));
}

ExtensionFunction::ResponseAction BraveSyncSyncSetupErrorFunction::Run() {
  LOG(ERROR) << "TAGAB BraveSyncSyncSetupErrorFunction::Run";
  std::unique_ptr<brave_sync::SyncSetupError::Params> params(
      brave_sync::SyncSetupError::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  LOG(ERROR) << "TAGAB BraveSyncSyncSetupErrorFunction::Run params->error=" << params->error;
  ::brave_sync::BraveSyncClient* sync_client = ::brave_sync::BraveSyncClientFactory::GetForBrowserContext(browser_context());
  sync_client->GetSyncToBrowserHandler()->OnSyncSetupError(params->error);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncSyncDebugFunction::Run() {
  LOG(ERROR) << "TAGAB BraveSyncSyncDebugFunction::Run";
  std::unique_ptr<brave_sync::SyncDebug::Params> params(
      brave_sync::SyncDebug::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  LOG(ERROR) << "TAGAB BraveSyncSyncDebugFunction::Run params->message=" << params->message;
  ::brave_sync::BraveSyncClient* sync_client = ::brave_sync::BraveSyncClientFactory::GetForBrowserContext(browser_context());
  sync_client->GetSyncToBrowserHandler()->OnSyncDebug(params->message);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncSaveInitDataFunction::Run() {
  LOG(ERROR) << "TAGAB BraveSyncSaveInitDataFunction::Run";
  std::unique_ptr<brave_sync::SaveInitData::Params> params(
      brave_sync::SaveInitData::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  if (params->seed) {
    LOG(ERROR) << "TAGAB BraveSyncSaveInitDataFunction::Run params->seed=" << params->seed->size();
  }
  if (params->device_id) {
    LOG(ERROR) << "TAGAB BraveSyncSaveInitDataFunction::Run params->device_id=" << params->device_id->size();
  }
  ::brave_sync::BraveSyncClient* sync_client = ::brave_sync::BraveSyncClientFactory::GetForBrowserContext(browser_context());
  sync_client->GetSyncToBrowserHandler()->OnSaveInitData(
    ::brave_sync::Uint8ArrayFromUnsignedCharVec(params->seed ? *params->seed : std::vector<uint8_t>()),
    ::brave_sync::Uint8ArrayFromUnsignedCharVec(params->device_id ? *params->device_id : std::vector<uint8_t>() )
  );

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncSyncReadyFunction::Run() {
  LOG(ERROR) << "TAGAB BraveSyncSyncReadyFunction::Run";

  ::brave_sync::BraveSyncClient* sync_client = ::brave_sync::BraveSyncClientFactory::GetForBrowserContext(browser_context());
  sync_client->GetSyncToBrowserHandler()->OnSyncReady();

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncGetExistingObjectsFunction::Run() {
  LOG(ERROR) << "TAGAB BraveSyncGetExistingObjectsFunction::Run";
  std::unique_ptr<brave_sync::GetExistingObjects::Params> params(
      brave_sync::GetExistingObjects::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  LOG(ERROR) << "TAGAB BraveSyncGetExistingObjectsFunction::Run params->category_name=" << params->category_name;
//  LOG(ERROR) << "TAGAB BraveSyncGetExistingObjectsFunction::Run params->records.size()=" << params->records.size();
  LOG(ERROR) << "TAGAB BraveSyncGetExistingObjectsFunction::Run params->last_record_timestamp=" << params->last_record_timestamp;
  LOG(ERROR) << "TAGAB BraveSyncGetExistingObjectsFunction::Run params->is_truncated=" << params->is_truncated;

  //std::vector<std::unique_ptr<base::Value>> records;

  // params->records of vector<extensions::api::brave_sync::SyncRecord>
  // to
  // const vector<brave_sync::SyncRecordPtr>

  auto records = std::make_unique<std::vector<::brave_sync::SyncRecordPtr>>();
  ::brave_sync::ConvertSyncRecords(params->records, *records.get());

  ::brave_sync::BraveSyncClient* sync_client = ::brave_sync::BraveSyncClientFactory::GetForBrowserContext(browser_context());
  sync_client->GetSyncToBrowserHandler()->OnGetExistingObjects(
    params->category_name,
    std::move(records),
    base::Time::FromJsTime(params->last_record_timestamp),
    params->is_truncated);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncResolvedSyncRecordsFunction::Run() {
  LOG(ERROR) << "TAGAB BraveSyncResolvedSyncRecordsFunction::Run";
  std::unique_ptr<brave_sync::ResolvedSyncRecords::Params> params(
      brave_sync::ResolvedSyncRecords::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  LOG(ERROR) << "TAGAB BraveSyncResolvedSyncRecordsFunction::Run params->category_name=" << params->category_name;
  LOG(ERROR) << "TAGAB BraveSyncResolvedSyncRecordsFunction::Run params->records.size()=" << params->records.size();


  auto records = std::make_unique<std::vector<::brave_sync::SyncRecordPtr>>();
  ::brave_sync::ConvertSyncRecords(params->records, *records.get());

  ::brave_sync::BraveSyncClient* sync_client = ::brave_sync::BraveSyncClientFactory::GetForBrowserContext(browser_context());
  sync_client->GetSyncToBrowserHandler()->OnResolvedSyncRecords(
    params->category_name,
    std::move(records));

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncSaveBookmarksBaseOrderFunction::Run() {
  LOG(ERROR) << "TAGAB BraveSyncSaveBookmarksBaseOrderFunction::Run";
  std::unique_ptr<brave_sync::SaveBookmarksBaseOrder::Params> params(
      brave_sync::SaveBookmarksBaseOrder::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  LOG(ERROR) << "TAGAB BraveSyncSaveBookmarksBaseOrderFunction::Run params->order=" << params->order;

  ::brave_sync::BraveSyncClient* sync_client = ::brave_sync::BraveSyncClientFactory::GetForBrowserContext(browser_context());
  sync_client->GetSyncToBrowserHandler()->OnSaveBookmarksBaseOrder(params->order);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncSaveBookmarkOrderFunction::Run() {
  LOG(ERROR) << "TAGAB BraveSyncSaveBookmarkOrderFunction::Run";
  std::unique_ptr<brave_sync::SaveBookmarkOrder::Params> params(
      brave_sync::SaveBookmarkOrder::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  LOG(ERROR) << "TAGAB BraveSyncSaveBookmarkOrderFunction::Run params->order=" << params->order;
  LOG(ERROR) << "TAGAB BraveSyncSaveBookmarkOrderFunction::Run params->prev_order=" << params->prev_order;
  LOG(ERROR) << "TAGAB BraveSyncSaveBookmarkOrderFunction::Run params->next_order=" << params->next_order;

  ::brave_sync::BraveSyncClient* sync_client = ::brave_sync::BraveSyncClientFactory::GetForBrowserContext(browser_context());
  sync_client->GetSyncToBrowserHandler()->OnSaveBookmarkOrder(params->order, params->prev_order, params->next_order);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncSyncWordsPreparedFunction::Run() {
  LOG(ERROR) << "TAGAB BraveSyncSyncWordsPreparedFunction::Run";
  std::unique_ptr<brave_sync::SyncWordsPrepared::Params> params(
      brave_sync::SyncWordsPrepared::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  LOG(ERROR) << "TAGAB BraveSyncSyncWordsPreparedFunction::Run params->words=" << params->words;

  ::brave_sync::BraveSyncClient* sync_client = ::brave_sync::BraveSyncClientFactory::GetForBrowserContext(browser_context());
  sync_client->GetSyncToBrowserHandler()->OnSyncWordsPrepared(params->words);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncBytesFromSyncWordsPreparedFunction::Run() {
  LOG(ERROR) << "TAGAB BraveSyncBytesFromSyncWordsPreparedFunction::Run";
  std::unique_ptr<brave_sync::BytesFromSyncWordsPrepared::Params> params(
      brave_sync::BytesFromSyncWordsPrepared::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  LOG(ERROR) << "TAGAB BraveSyncSyncWordsPreparedFunction::Run params->bytes.size()=" << params->bytes.size();
  LOG(ERROR) << "TAGAB BraveSyncSyncWordsPreparedFunction::Run params->error_message=" << params->error_message;

  ::brave_sync::BraveSyncClient* sync_client = ::brave_sync::BraveSyncClientFactory::GetForBrowserContext(browser_context());
  sync_client->GetSyncToBrowserHandler()->OnBytesFromSyncWordsPrepared(
    ::brave_sync::Uint8ArrayFromUnsignedCharVec(params->bytes), params->error_message);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncExtensionLoadedFunction::Run() {
  LOG(ERROR) << "TAGAB BraveSyncExtensionLoadedFunction::Run";

  // Also inform sync client extension started
  ::brave_sync::BraveSyncClient* sync_client = ::brave_sync::BraveSyncClientFactory::GetForBrowserContext(browser_context());
  sync_client->ExtensionStartupComplete();

  return RespondNow(NoArguments());
}

}  // namespace api
}  // namespace extensions
