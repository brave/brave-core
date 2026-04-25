// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SAFARI_DATA_IMPORT_SAFARI_DATA_IMPORT_CLIENT_DELEGATE_H_
#define BRAVE_IOS_BROWSER_SAFARI_DATA_IMPORT_SAFARI_DATA_IMPORT_CLIENT_DELEGATE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol SafariDataImportClientDelegate

// Phase one: parsing data

/// Triggered when the import fails entirely, e.g., due to an invalid file.
- (void)onTotalFailure;

/// Invoked when the number of bookmarks in the input file has been determined.
- (void)onBookmarksReady:(NSInteger)count;

/// Invoked when the number of history items in the input file has been
/// determined. Unlike other data types, this is an estimate and not an exact
/// count. An input file may contain one history file per Safari profile.
- (void)onHistoryReady:(NSInteger)estimatedCount;

/// Invoked when the number of passwords in the input file has been determined.
/// The conflictedPasswordIDs array provides IDs of passwords with a
/// conflict (i.e., those where the user already has a
/// different saved password for the same username/URL); the Client must use
/// this information to resolve conflicts and continue the import flow.
- (void)onPasswordsReady:(NSArray<NSNumber*>*)conflictedPasswordIDs;

/// Invoked when the number of payment cards in the input file has been
/// determined.
- (void)onPaymentCardsReady:(NSInteger)count;

// Phase two: executing import

/// Invoked when importing of bookmarks has completed. `count` is the number
/// which were successfully imported.
- (void)onBookmarksImported:(NSInteger)count;

/// Invoked when importing of history has completed. `count` is the number of
/// entries which were successfully imported.
- (void)onHistoryImported:(NSInteger)count;

/// Invoked when importing of passwords has completed. `count` is the number
/// which were successfully imported.
- (void)onPasswordsImported:(NSInteger)count;

/// Invoked when importing of payment cards has completed. `count` is the
/// number which were successfully imported.
- (void)onPaymentCardsImported:(NSInteger)count;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_SAFARI_DATA_IMPORT_SAFARI_DATA_IMPORT_CLIENT_DELEGATE_H_
