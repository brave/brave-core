/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LogServiceApiAction LogServiceApiAction_ChromiumImpl

#include "../../../../../../components/download/internal/background_service/stats.cc"
#undef LogServiceApiAction

void LogServiceApiAction(DownloadClient client, ServiceApiAction action) {
  if (client == DownloadClient::CUSTOM_LIST_SUBSCRIPTIONS)
    return;
  LogServiceApiAction_ChromiumImpl(client, action);
}
