/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/request_type.h"

#define BRAVE_GET_ICON_ID_DESKTOP \
  case RequestType::kWidevine:    \
    return vector_icons::kExtensionIcon;

#include "../../../../components/permissions/request_type.cc"
#undef BRAVE_GET_ICON_ID_DESKTOP
