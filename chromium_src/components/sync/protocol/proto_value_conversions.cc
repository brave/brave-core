/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
 
#include "brave/components/sync/protocol/vg_specifics.pb.h"

#define BRAVE_IMPLEMENT_PROTO_TO_VALUE_VG_BODY_SPECIFICS_VG_SPEND_STATUS_SPECIFICS \
IMPLEMENT_PROTO_TO_VALUE(VgBodySpecifics)                                          \
IMPLEMENT_PROTO_TO_VALUE(VgSpendStatusSpecifics)
#include "src/components/sync/protocol/proto_value_conversions.cc"
#undef BRAVE_IMPLEMENT_PROTO_TO_VALUE_VG_BODY_SPECIFICS_VG_SPEND_STATUS_SPECIFICS
