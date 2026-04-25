// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SAFARI_DATA_IMPORT_SAFARI_DATA_IMPORTER_COORDINATOR_H_
#define BRAVE_IOS_BROWSER_SAFARI_DATA_IMPORT_SAFARI_DATA_IMPORTER_COORDINATOR_H_

#import <Foundation/Foundation.h>

@protocol ProfileBridge;
@protocol SafariDataImportClientDelegate;
@protocol SafariDataImporterBridge;

NS_ASSUME_NONNULL_BEGIN

@protocol SafariDataImporterCoordinator

/// The Safari data importer that performs the actual import operations.
/// This importer handles parsing and importing bookmarks, passwords, history,
/// and payment cards from Safari export files.
@property(readonly) id<SafariDataImporterBridge> importer;

/// Delegate that receives callbacks during the import process.
/// The delegate is notified when data is ready for import and when import
/// operations complete, allowing the UI to update accordingly.
@property(nonatomic, weak) id<SafariDataImportClientDelegate> delegate;

@end

/// Concrete implementation of SafariDataImporterCoordinator.
/// Creates and manages the Safari data importer with all required dependencies
/// from the provided Chrome profile, including password stores, bookmark model,
/// history service, and other Chrome services needed for import operations.
OBJC_EXPORT
@interface SafariDataImporterCoordinatorImpl
    : NSObject <SafariDataImporterCoordinator>
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithProfile:(id<ProfileBridge>)profile
    NS_DESIGNATED_INITIALIZER;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_SAFARI_DATA_IMPORT_SAFARI_DATA_IMPORTER_COORDINATOR_H_
