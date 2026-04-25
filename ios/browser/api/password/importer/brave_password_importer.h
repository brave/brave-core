// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_PASSWORD_IMPORTER_BRAVE_PASSWORD_IMPORTER_H_
#define BRAVE_IOS_BROWSER_API_PASSWORD_IMPORTER_BRAVE_PASSWORD_IMPORTER_H_

#include <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NSInteger BravePasswordImportEntryStatus
    NS_TYPED_ENUM NS_SWIFT_NAME(BravePasswordImportEntry.Status);

OBJC_EXPORT BravePasswordImportEntryStatus const
    BravePasswordImportEntryStatusNone;
OBJC_EXPORT BravePasswordImportEntryStatus const
    BravePasswordImportEntryStatusUnknownError;
OBJC_EXPORT BravePasswordImportEntryStatus const
    BravePasswordImportEntryStatusMissingPassword;
OBJC_EXPORT BravePasswordImportEntryStatus const
    BravePasswordImportEntryStatusMissingURL;
OBJC_EXPORT BravePasswordImportEntryStatus const
    BravePasswordImportEntryStatusInvalidURL;
OBJC_EXPORT BravePasswordImportEntryStatus const
    BravePasswordImportEntryStatusLongURL;
OBJC_EXPORT BravePasswordImportEntryStatus const
    BravePasswordImportEntryStatusLongPassword;
OBJC_EXPORT BravePasswordImportEntryStatus const
    BravePasswordImportEntryStatusLongUsername;
OBJC_EXPORT BravePasswordImportEntryStatus const
    BravePasswordImportEntryStatusConflictProfile;
OBJC_EXPORT BravePasswordImportEntryStatus const
    BravePasswordImportEntryStatusConflictAccount;
OBJC_EXPORT BravePasswordImportEntryStatus const
    BravePasswordImportEntryStatusLongNote;
OBJC_EXPORT BravePasswordImportEntryStatus const
    BravePasswordImportEntryStatusLongConcatenatedNote;
OBJC_EXPORT BravePasswordImportEntryStatus const
    BravePasswordImportEntryStatusValid;

OBJC_EXPORT
@interface BravePasswordImportEntry : NSObject
@property(nonatomic, readonly) BravePasswordImportEntryStatus status;
@property(nonatomic, readonly) NSInteger id;
@property(nonatomic, readonly) NSString* username;
@property(nonatomic, readonly) NSString* password;
@end

typedef NSInteger BravePasswordImporterResultsStatus
    NS_TYPED_ENUM NS_SWIFT_NAME(BravePasswordImporterResults.Status);

OBJC_EXPORT BravePasswordImporterResultsStatus const
    BravePasswordImporterResultsStatusNone;
OBJC_EXPORT BravePasswordImporterResultsStatus const
    BravePasswordImporterResultsStatusUnknownError;
OBJC_EXPORT BravePasswordImporterResultsStatus const
    BravePasswordImporterResultsStatusSuccess;
OBJC_EXPORT BravePasswordImporterResultsStatus const
    BravePasswordImporterResultsStatusIOError;
OBJC_EXPORT BravePasswordImporterResultsStatus const
    BravePasswordImporterResultsStatusBadFormat;
OBJC_EXPORT BravePasswordImporterResultsStatus const
    BravePasswordImporterResultsStatusDismissed;
OBJC_EXPORT BravePasswordImporterResultsStatus const
    BravePasswordImporterResultsStatusMaxFileSize;
OBJC_EXPORT BravePasswordImporterResultsStatus const
    BravePasswordImporterResultsStatusImportAlreadyActive;
OBJC_EXPORT BravePasswordImporterResultsStatus const
    BravePasswordImporterResultsStatusNumPasswordsExceeded;
OBJC_EXPORT BravePasswordImporterResultsStatus const
    BravePasswordImporterResultsStatusConflicts;

OBJC_EXPORT
NS_SWIFT_NAME(BravePasswordImporter.Results)
@interface BravePasswordImporterResults : NSObject
@property(nonatomic, readonly) BravePasswordImporterResultsStatus status;
@property(nonatomic, readonly) NSString* fileName;
@property(nonatomic, readonly) NSUInteger numberImported;
@property(nonatomic, readonly)
    NSArray<BravePasswordImportEntry*>* displayedEntries;
@end

OBJC_EXPORT
@interface BravePasswordImporter : NSObject
- (void)importPasswords:(NSString*)fileName
             completion:(void (^)(BravePasswordImporterResults*))completion;

- (void)continueImport:(NSArray<BravePasswordImportEntry*>*)entriesToReplace
            completion:(void (^)(BravePasswordImporterResults*))completion;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_PASSWORD_IMPORTER_BRAVE_PASSWORD_IMPORTER_H_
