// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/safari_data_import/safari_data_importer_bridge_impl.h"

#include "base/files/file_path.h"
#include "base/strings/sys_string_conversions.h"
#include "components/user_data_importer/utility/safari_data_importer.h"

@implementation SafariDataImporterBridgeImpl {
  std::unique_ptr<user_data_importer::SafariDataImporter> _importer;
}

- (instancetype)initWithSafariDataImporter:
    (std::unique_ptr<user_data_importer::SafariDataImporter>)importer {
  if ((self = [super init])) {
    _importer = std::move(importer);
  }
  return self;
}

- (void)prepareImportForFileAtPath:(NSString*)path {
  _importer->PrepareImport(base::FilePath(base::SysNSStringToUTF8(path)));
}

- (void)completeImportWithSelectedPasswords:
    (nullable NSArray<NSNumber*>*)selectedPasswordIDs {
  std::vector<int> passwordIDs;
  if (selectedPasswordIDs && selectedPasswordIDs.count > 0) {
    for (NSNumber* identifier in selectedPasswordIDs) {
      passwordIDs.push_back([identifier intValue]);
    }
  }
  _importer->CompleteImport(passwordIDs);
}

- (void)cancelImport {
  _importer->CancelImport();
}

@end
