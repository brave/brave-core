/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/chrome/browser/download/model/background_service/background_download_service_factory.h"

#include <map>

#include "components/download/internal/background_service/init_aware_background_download_service.h"
#include "components/download/internal/background_service/proto/entry.pb.h"
#include "components/leveldb_proto/public/proto_database_provider.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

// Do not download prediction models
#define insert(PAIR)                                                     \
  insert(PAIR);                                                          \
  if (PAIR.first ==                                                      \
      download::DownloadClient::OPTIMIZATION_GUIDE_PREDICTION_MODELS) {  \
    clients->erase(                                                      \
        download::DownloadClient::OPTIMIZATION_GUIDE_PREDICTION_MODELS); \
  }
#include <ios/chrome/browser/download/model/background_service/background_download_service_factory.cc>
#undef insert
