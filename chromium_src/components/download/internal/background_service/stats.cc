/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_CLIENT_TO_HISTOGRAM_SUFFIX          \
  case DownloadClient::CUSTOM_LIST_SUBSCRIPTIONS: \
    return "CustomListSubscriptions";
#include "../../../../../../components/download/internal/background_service/stats.cc"
