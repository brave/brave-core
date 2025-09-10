// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import "brave/ios/browser/safari_data_import/safari_data_import_client_bridge.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/safari_data_import/safari_data_import_client_delegate.h"
#include "components/password_manager/core/browser/import/import_results.h"

BraveSafariDataImportClientBridge::BraveSafariDataImportClientBridge() =
    default;

BraveSafariDataImportClientBridge::~BraveSafariDataImportClientBridge() =
    default;

void BraveSafariDataImportClientBridge::OnTotalFailure() {
  if (delegate_) {
    [delegate_ onTotalFailure];
  }
}

void BraveSafariDataImportClientBridge::OnBookmarksReady(
    user_data_importer::CountOrError result) {
  if (delegate_) {
    [delegate_ onBookmarksReady:static_cast<NSInteger>(
                                    result.has_value() ? result.value() : 0)];
  }
}

void BraveSafariDataImportClientBridge::OnHistoryReady(
    user_data_importer::CountOrError estimated_count) {
  if (delegate_) {
    [delegate_
        onHistoryReady:static_cast<NSInteger>(estimated_count.has_value()
                                                  ? estimated_count.value()
                                                  : 0)];
  }
}

void BraveSafariDataImportClientBridge::OnPasswordsReady(
    base::expected<password_manager::ImportResults,
                   user_data_importer::ImportPreparationError> results) {
  if (delegate_) {
    NSMutableArray<NSNumber*>* ids = [[NSMutableArray alloc] init];
    if (results.has_value() &&
        results.value().status ==
            password_manager::ImportResults::Status::CONFLICTS) {
      for (const auto& entry : results.value().displayed_entries) {
        [ids addObject:@(entry.id)];
      }
    }
    [delegate_ onPasswordsReady:ids];
  }
}

void BraveSafariDataImportClientBridge::OnPaymentCardsReady(
    user_data_importer::CountOrError result) {
  if (delegate_) {
    [delegate_ onPaymentCardsReady:static_cast<NSInteger>(result.has_value()
                                                              ? result.value()
                                                              : 0)];
  }
}

void BraveSafariDataImportClientBridge::OnBookmarksImported(size_t count) {
  if (delegate_) {
    [delegate_ onBookmarksImported:static_cast<NSInteger>(count)];
  }
}

void BraveSafariDataImportClientBridge::OnHistoryImported(size_t count) {
  if (delegate_) {
    [delegate_ onHistoryImported:static_cast<NSInteger>(count)];
  }
}

void BraveSafariDataImportClientBridge::OnPasswordsImported(
    const password_manager::ImportResults& results) {
  if (delegate_) {
    size_t imported_count = 0;
    for (const auto& entry : results.displayed_entries) {
      if (entry.status == password_manager::ImportEntry::Status::VALID) {
        imported_count++;
      }
    }
    [delegate_ onPasswordsImported:static_cast<NSInteger>(imported_count)];
  }
}

void BraveSafariDataImportClientBridge::OnPaymentCardsImported(size_t count) {
  if (delegate_) {
    [delegate_ onPaymentCardsImported:static_cast<NSInteger>(count)];
  }
}

base::WeakPtr<user_data_importer::SafariDataImportClient>
BraveSafariDataImportClientBridge::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}
