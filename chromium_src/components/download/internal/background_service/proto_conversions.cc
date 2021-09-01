/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_DOWNLOAD_CLIENT_TO_PROTO            \
  case DownloadClient::CUSTOM_LIST_SUBSCRIPTIONS: \
    return protodb::DownloadClient::CUSTOM_LIST_SUBSCRIPTIONS;

#define BRAVE_DOWNLOAD_CLIENT_FROM_PROTO                   \
  case protodb::DownloadClient::CUSTOM_LIST_SUBSCRIPTIONS: \
    return DownloadClient::CUSTOM_LIST_SUBSCRIPTIONS;

#include "../../../../../../components/download/internal/background_service/proto_conversions.cc"
