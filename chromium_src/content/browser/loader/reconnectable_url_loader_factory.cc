/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/services/network/public/cpp/system_request_handler.h"

// ReconnectableURLLoaderFactory is only used for requests made outside of a
// renderer context so while this is attached to the profile, it calls
// OnBeforeSystemRequest because they are background requests made by the
// browser on behalf of the user
#define BRAVE_CREATE_LOADER_AND_START                                  \
  network::SystemRequestHandler::GetInstance()->OnBeforeSystemRequest( \
      url_request)

#include <content/browser/loader/reconnectable_url_loader_factory.cc>

#undef BRAVE_CREATE_LOADER_AND_START
