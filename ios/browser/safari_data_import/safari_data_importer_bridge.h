// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SAFARI_DATA_IMPORT_SAFARI_DATA_IMPORTER_BRIDGE_H_
#define BRAVE_IOS_BROWSER_SAFARI_DATA_IMPORT_SAFARI_DATA_IMPORTER_BRIDGE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(SafariDataImporter)
@protocol SafariDataImporterBridge

/// Opens the ZIP file provided in `path` and prepares to import the data
/// (passwords, payment cards, bookmarks, and history) contained inside. Each
/// data type is optional and may or may not be present.
- (void)prepareImportForFileAtPath:(NSString*)path;

/// Called after calling `PrepareImport` in order to complete the import
/// process. In case of password conflicts, `selected_password_ids` provides
/// the list of conflicting passwords to import.
- (void)completeImportWithSelectedPasswords:
    (nullable NSArray<NSNumber*>*)selectedPasswordIDs;

/// Called after calling "Import" in order to cancel the import process.
- (void)cancelImport;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_SAFARI_DATA_IMPORT_SAFARI_DATA_IMPORTER_BRIDGE_H_
