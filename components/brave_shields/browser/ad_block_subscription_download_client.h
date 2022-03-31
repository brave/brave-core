/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_DOWNLOAD_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_DOWNLOAD_CLIENT_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "components/download/public/background_service/client.h"
#include "components/download/public/background_service/download_metadata.h"

class Profile;

namespace brave_shields {

class AdBlockSubscriptionServiceManager;

class AdBlockSubscriptionDownloadManager;

class AdBlockSubscriptionDownloadClient : public download::Client {
 public:
  AdBlockSubscriptionDownloadClient(
      AdBlockSubscriptionServiceManager* subscription_manager);
  ~AdBlockSubscriptionDownloadClient() override;
  AdBlockSubscriptionDownloadClient(const AdBlockSubscriptionDownloadClient&) =
      delete;
  AdBlockSubscriptionDownloadClient& operator=(
      const AdBlockSubscriptionDownloadClient&) = delete;

  // download::Client:
  void OnServiceInitialized(
      bool state_lost,
      const std::vector<download::DownloadMetaData>& downloads) override;
  void OnServiceUnavailable() override;
  void OnDownloadFailed(const std::string& guid,
                        const download::CompletionInfo& completion_info,
                        download::Client::FailureReason reason) override;
  void OnDownloadSucceeded(
      const std::string& guid,
      const download::CompletionInfo& completion_info) override;
  bool CanServiceRemoveDownloadedFile(const std::string& guid,
                                      bool force_delete) override;
  void GetUploadData(const std::string& guid,
                     download::GetUploadDataCallback callback) override;

 private:
  // Returns the AdBlockSubscriptionDownloadManager for the profile.
  AdBlockSubscriptionDownloadManager* GetAdBlockSubscriptionDownloadManager();

  raw_ptr<AdBlockSubscriptionServiceManager> subscription_manager_ =
      nullptr;  // NOT OWNED
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_DOWNLOAD_CLIENT_H_
