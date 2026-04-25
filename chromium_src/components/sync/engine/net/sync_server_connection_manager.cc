/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/http/http_request_headers.h"

// At Connection::PostRequestAndDownloadResponse upstream expects access_token
// just as a single string with a single token.
// We add own authorization, so `access_token` for us looks as
// `token==\r\nBraveServiceKey: braveToken`, see
// `BraveSyncAuthManager::GenerateAccessToken`. The new upstream implementation
// is not tolerant to `\r\n`.
// The override here below uses
// `HttpRequestHeaders::AddHeadersFromString` which splits the headers separated
// with `\r\n`. So idea of this override is just to act as this patch:
// -    headers.SetHeader("Authorization", "Bearer " + access_token);
// +    headers.AddHeadersFromString("Authorization: Bearer " + access_token);

#define SetHeader(ARG1, ARG2)                                  \
  AddHeadersFromString(std::string(ARG1) + std::string(": ") + \
                       std::string(ARG2))

#include <components/sync/engine/net/sync_server_connection_manager.cc>

#undef SetHeader
