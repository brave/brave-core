// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SAFARI_DATA_IMPORT_SAFARI_DATA_IMPORT_CLIENT_BRIDGE_H_
#define BRAVE_IOS_BROWSER_SAFARI_DATA_IMPORT_SAFARI_DATA_IMPORT_CLIENT_BRIDGE_H_

#include "base/memory/weak_ptr.h"
#include "components/user_data_importer/utility/safari_data_import_client.h"

@protocol SafariDataImportClientDelegate;

class BraveSafariDataImportClientBridge
    : public user_data_importer::SafariDataImportClient {
 public:
  BraveSafariDataImportClientBridge();
  ~BraveSafariDataImportClientBridge() override;

  id<SafariDataImportClientDelegate> delegate() { return delegate_; }
  void SetDelegate(id<SafariDataImportClientDelegate> delegate) {
    delegate_ = delegate;
  }

  // SafariDataImportClient implementation
  void OnTotalFailure() override;
  void OnBookmarksReady(user_data_importer::CountOrError result) override;
  void OnHistoryReady(
      user_data_importer::CountOrError estimated_count) override;
  void OnPasswordsReady(
      base::expected<password_manager::ImportResults,
                     user_data_importer::ImportPreparationError> results)
      override;
  void OnPaymentCardsReady(user_data_importer::CountOrError result) override;
  void OnBookmarksImported(size_t count) override;
  void OnHistoryImported(size_t count) override;
  void OnPasswordsImported(
      const password_manager::ImportResults& results) override;
  void OnPaymentCardsImported(size_t count) override;
  base::WeakPtr<user_data_importer::SafariDataImportClient> AsWeakPtr()
      override;

 private:
  __weak id<SafariDataImportClientDelegate> delegate_ = nullptr;
  base::WeakPtrFactory<BraveSafariDataImportClientBridge> weak_factory_{this};
};

#endif  // BRAVE_IOS_BROWSER_SAFARI_DATA_IMPORT_SAFARI_DATA_IMPORT_CLIENT_BRIDGE_H_
