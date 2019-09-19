/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SYNC_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SYNC_API_H_

#include "extensions/browser/extension_function.h"

namespace extensions {
namespace api {

class BraveSyncGetInitDataFunction : public ExtensionFunction {
  ~BraveSyncGetInitDataFunction() override {}
  DECLARE_EXTENSION_FUNCTION("braveSync.getInitData", UNKNOWN)
  ResponseAction Run() override;
};

class BraveSyncSyncSetupErrorFunction : public ExtensionFunction {
  ~BraveSyncSyncSetupErrorFunction() override {}
  DECLARE_EXTENSION_FUNCTION("braveSync.syncSetupError", UNKNOWN)
  ResponseAction Run() override;
};

class BraveSyncSyncDebugFunction : public ExtensionFunction {
  ~BraveSyncSyncDebugFunction() override {}
  DECLARE_EXTENSION_FUNCTION("braveSync.syncDebug", UNKNOWN)
  ResponseAction Run() override;
};

class BraveSyncSaveInitDataFunction : public ExtensionFunction {
  ~BraveSyncSaveInitDataFunction() override {}
  DECLARE_EXTENSION_FUNCTION("braveSync.saveInitData", UNKNOWN)
  ResponseAction Run() override;
};

class BraveSyncSyncReadyFunction : public ExtensionFunction {
  ~BraveSyncSyncReadyFunction() override {}
  DECLARE_EXTENSION_FUNCTION("braveSync.syncReady", UNKNOWN)
  ResponseAction Run() override;
};

class BraveSyncGetExistingObjectsFunction : public ExtensionFunction {
  ~BraveSyncGetExistingObjectsFunction() override {}
  DECLARE_EXTENSION_FUNCTION("braveSync.getExistingObjects", UNKNOWN)
  ResponseAction Run() override;
};

class BraveSyncResolvedSyncRecordsFunction : public ExtensionFunction {
  ~BraveSyncResolvedSyncRecordsFunction() override {}
  DECLARE_EXTENSION_FUNCTION("braveSync.resolvedSyncRecords", UNKNOWN)
  ResponseAction Run() override;
};

class BraveSyncSaveBookmarksBaseOrderFunction : public ExtensionFunction {
  ~BraveSyncSaveBookmarksBaseOrderFunction() override {}
  DECLARE_EXTENSION_FUNCTION("braveSync.saveBookmarksBaseOrder", UNKNOWN)
  ResponseAction Run() override;
};

class BraveSyncExtensionInitializedFunction : public ExtensionFunction {
  ~BraveSyncExtensionInitializedFunction() override {}
  DECLARE_EXTENSION_FUNCTION("braveSync.extensionInitialized", UNKNOWN)
  ResponseAction Run() override;
};

}   // namespace api
}   // namespace extensions

#endif    // BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SYNC_API_H_
