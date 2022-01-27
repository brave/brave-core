/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SYNC_DRIVER_BRAVE_SYNC_STOPPED_REPORTER_H_
#define BRAVE_COMPONENTS_SYNC_DRIVER_BRAVE_SYNC_STOPPED_REPORTER_H_

#include <string>

#include "components/sync/driver/sync_stopped_reporter.h"

namespace syncer {

class BraveSyncStoppedReporter : public SyncStoppedReporter {
 public:
  BraveSyncStoppedReporter(
      const GURL& sync_service_url,
      const std::string& user_agent,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      ResultCallback callback);
  BraveSyncStoppedReporter(const BraveSyncStoppedReporter&) = delete;
  BraveSyncStoppedReporter& operator=(const BraveSyncStoppedReporter&) = delete;
  ~BraveSyncStoppedReporter() override;

  void ReportSyncStopped(const std::string& access_token,
                         const std::string& cache_guid,
                         const std::string& birthday) override;
};

}  // namespace syncer

#endif  // BRAVE_COMPONENTS_SYNC_DRIVER_BRAVE_SYNC_STOPPED_REPORTER_H_
