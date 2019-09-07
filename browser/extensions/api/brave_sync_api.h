/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SYNC_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SYNC_API_H_

#include "extensions/browser/extension_function.h"

namespace extensions {
namespace api {

class BraveSyncGetInitDataFunction : public UIThreadExtensionFunction {
  ~BraveSyncGetInitDataFunction() override {}
  DECLARE_EXTENSION_FUNCTION("braveSync.getInitData", UNKNOWN)
  ResponseAction Run() override;
};

class BraveSyncSyncSetupErrorFunction : public UIThreadExtensionFunction {
  ~BraveSyncSyncSetupErrorFunction() override {}
  DECLARE_EXTENSION_FUNCTION("braveSync.syncSetupError", UNKNOWN)
  ResponseAction Run() override;
};

class BraveSyncSyncDebugFunction : public UIThreadExtensionFunction {
  ~BraveSyncSyncDebugFunction() override {}
  DECLARE_EXTENSION_FUNCTION("braveSync.syncDebug", UNKNOWN)
  ResponseAction Run() override;
};

class BraveSyncSaveInitDataFunction : public UIThreadExtensionFunction {
  ~BraveSyncSaveInitDataFunction() override {}
  DECLARE_EXTENSION_FUNCTION("braveSync.saveInitData", UNKNOWN)
  ResponseAction Run() override;
};

class BraveSyncSyncReadyFunction : public UIThreadExtensionFunction {
  ~BraveSyncSyncReadyFunction() override {}
  DECLARE_EXTENSION_FUNCTION("braveSync.syncReady", UNKNOWN)
  ResponseAction Run() override;
};

class BraveSyncGetExistingObjectsFunction : public UIThreadExtensionFunction {
  ~BraveSyncGetExistingObjectsFunction() override {}
  DECLARE_EXTENSION_FUNCTION("braveSync.getExistingObjects", UNKNOWN)
  ResponseAction Run() override;
};

class BraveSyncResolvedSyncRecordsFunction : public UIThreadExtensionFunction {
  ~BraveSyncResolvedSyncRecordsFunction() override {}
  DECLARE_EXTENSION_FUNCTION("braveSync.resolvedSyncRecords", UNKNOWN)
  ResponseAction Run() override;
};

class BraveSyncSaveBookmarksBaseOrderFunction
    : public UIThreadExtensionFunction {
  ~BraveSyncSaveBookmarksBaseOrderFunction() override {}
  DECLARE_EXTENSION_FUNCTION("braveSync.saveBookmarksBaseOrder", UNKNOWN)
  ResponseAction Run() override;
};

class BraveSyncExtensionInitializedFunction
    : public UIThreadExtensionFunction {
  ~BraveSyncExtensionInitializedFunction() override {}
  DECLARE_EXTENSION_FUNCTION("braveSync.extensionInitialized", UNKNOWN)
  ResponseAction Run() override;
};

}   // namespace api
}   // namespace extensions

#endif    // BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SYNC_API_H_
