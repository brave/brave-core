// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SAFARI_DATA_IMPORT_SAFARI_DATA_IMPORTER_BRIDGE_IMPL_H_
#define BRAVE_IOS_BROWSER_SAFARI_DATA_IMPORT_SAFARI_DATA_IMPORTER_BRIDGE_IMPL_H_

#import <Foundation/Foundation.h>

#include <memory>

#include "brave/ios/browser/safari_data_import/safari_data_importer_bridge.h"

namespace user_data_importer {
class SafariDataImporter;
}

NS_ASSUME_NONNULL_BEGIN

@interface SafariDataImporterBridgeImpl : NSObject <SafariDataImporterBridge>
- (instancetype)initWithSafariDataImporter:
    (std::unique_ptr<user_data_importer::SafariDataImporter>)importer;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_SAFARI_DATA_IMPORT_SAFARI_DATA_IMPORTER_BRIDGE_IMPL_H_
