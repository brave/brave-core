/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "content/browser/devtools/protocol/network_handler.h"

#define BRAVE_COOKIE_RETRIEVER_NETWORK_SERVICE_RETRIEVE_ARGS \
  const url::Origin &top_frame_origin,

#define BRAVE_COOKIE_RETRIEVER_NETWORK_SERVICE_RETRIEVE_BODY \
  cookie_options.set_top_frame_origin(top_frame_origin);

#define BRAVE_NETWORK_HANDLER_GET_COOKIES_RETREIVE_CALL_ARGS \
  host_->ComputeTopFrameOrigin(host_->GetLastCommittedOrigin()),

#define BRAVE_NETWORK_HANDLER_SET_COOKIES_SET_COOKIE_OPTIONS                 \
  options.set_top_frame_origin(                                              \
      host_ ? base::make_optional<url::Origin>(host_->ComputeTopFrameOrigin( \
                  host_->GetLastCommittedOrigin()))                          \
            : base::nullopt);

#include "../../../../../../content/browser/devtools/protocol/network_handler.cc"

#undef BRAVE_NETWORK_HANDLER_SET_COOKIES_SET_COOKIE_OPTIONS
#undef BRAVE_NETWORK_HANDLER_GET_COOKIES_RETREIVE_CALL_ARGS
#undef BRAVE_COOKIE_RETRIEVER_NETWORK_SERVICE_RETRIEVE_BODY
#undef BRAVE_COOKIE_RETRIEVER_NETWORK_SERVICE_RETRIEVE_ARGS
