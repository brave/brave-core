/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_sync_api.h"

#include <memory>
#include <utility>
#include <vector>

#include "brave/common/extensions/api/brave_sync.h"
#include "brave/components/brave_sync/brave_profile_sync_service.h"
#include "brave/components/brave_sync/client/brave_sync_client.h"
#include "brave/components/brave_sync/client/client_ext_impl_data.h"
#include "brave/components/brave_sync/values_conv.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"

using brave_sync::BraveProfileSyncService;
using content::BrowserContext;

namespace extensions {
namespace api {

namespace {

BraveProfileSyncService* GetProfileSyncService(
    BrowserContext* browser_context) {
  return ProfileSyncServiceFactory::GetAsBraveProfileSyncServiceForProfile(
      Profile::FromBrowserContext(browser_context));
}

}  // namespace
ExtensionFunction::ResponseAction BraveSyncGetInitDataFunction::Run() {
  std::unique_ptr<brave_sync::GetInitData::Params> params(
      brave_sync::GetInitData::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  BraveProfileSyncService* sync_service =
    GetProfileSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->OnGetInitData(params->sync_version);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncSyncSetupErrorFunction::Run() {
  std::unique_ptr<brave_sync::SyncSetupError::Params> params(
      brave_sync::SyncSetupError::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  BraveProfileSyncService* sync_service =
    GetProfileSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->OnSyncSetupError(
      params->error);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncSyncDebugFunction::Run() {
  std::unique_ptr<brave_sync::SyncDebug::Params> params(
      brave_sync::SyncDebug::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  BraveProfileSyncService* sync_service =
    GetProfileSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->OnSyncDebug(
      params->message);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncSaveInitDataFunction::Run() {
  std::unique_ptr<brave_sync::SaveInitData::Params> params(
      brave_sync::SaveInitData::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  BraveProfileSyncService* sync_service =
    GetProfileSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->OnSaveInitData(
      params->seed ? *params->seed : std::vector<uint8_t>(),
      params->device_id ? *params->device_id : std::vector<uint8_t>());

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncSyncReadyFunction::Run() {
  BraveProfileSyncService* sync_service =
    GetProfileSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->OnSyncReady();

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncGetExistingObjectsFunction::Run() {
  std::unique_ptr<brave_sync::GetExistingObjects::Params> params(
      brave_sync::GetExistingObjects::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto records = std::make_unique<std::vector<::brave_sync::SyncRecordPtr>>();
  ::brave_sync::ConvertSyncRecords(params->records, *records.get());

  BraveProfileSyncService* sync_service =
    GetProfileSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->OnGetExistingObjects(
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

  BraveProfileSyncService* sync_service =
    GetProfileSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->OnResolvedSyncRecords(
    params->category_name,
    std::move(records));

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
BraveSyncSaveBookmarksBaseOrderFunction::Run() {
  std::unique_ptr<brave_sync::SaveBookmarksBaseOrder::Params> params(
      brave_sync::SaveBookmarksBaseOrder::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  BraveProfileSyncService* sync_service =
    GetProfileSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->OnSaveBookmarksBaseOrder(
      params->order);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncSyncWordsPreparedFunction::Run() {
  std::unique_ptr<brave_sync::SyncWordsPrepared::Params> params(
      brave_sync::SyncWordsPrepared::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  BraveProfileSyncService* sync_service =
    GetProfileSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->OnSyncWordsPrepared(params->words);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveSyncExtensionInitializedFunction::Run() {
  // Also inform sync client extension started
  BraveProfileSyncService* sync_service =
    GetProfileSyncService(browser_context());
  DCHECK(sync_service);
  sync_service->GetBraveSyncClient()->OnExtensionInitialized();

  return RespondNow(NoArguments());
}

}  // namespace api
}  // namespace extensions
