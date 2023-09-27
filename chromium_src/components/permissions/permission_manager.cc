/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_GET_PERMISSION_RESULT_FOR_CURRENT_DOCUMENT \
  !forced_requesting_origin_.is_empty() ? forced_requesting_origin_:

#define BRAVE_REQUEST_PERMISSION_FROM_CURRENT_DOCUMENT                 \
  if (!forced_requesting_origin_.is_empty()) {                         \
    auto desc = std::move(request_description);                        \
    desc.requesting_origin = forced_requesting_origin_;                \
    RequestPermissionsInternal(render_frame_host, desc,                \
                               std::move(permission_status_callback)); \
    return;                                                            \
  }

#include "src/components/permissions/permission_manager.cc"

#undef BRAVE_GET_PERMISSION_RESULT_FOR_CURRENT_DOCUMENT
#undef BRAVE_REQUEST_PERMISSION_FROM_CURRENT_DOCUMENT
