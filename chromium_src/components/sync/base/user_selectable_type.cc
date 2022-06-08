/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
 
#include "components/sync/base/features.h"
#include "components/sync/base/pref_names.h"
#include "components/sync/base/user_selectable_type.h"

#define syncer syncer {}                                         \
namespace {                                                      \
  constexpr char kVgBodiesTypeName[] = "vgBodies";               \
  constexpr char kVgSpendStatusesTypeName[] = "vgSpendStatuses"; \
}                                                                \
                                                                 \
namespace syncer

#define USER_EVENTS USER_EVENTS}};                    \
case UserSelectableType::kVgBodies:                   \
  return {kVgBodiesTypeName, VG_BODIES, {VG_BODIES}}; \
case UserSelectableType::kVgSpendStatuses:            \
  return {kVgSpendStatusesTypeName, VG_SPEND_STATUSES, {VG_SPEND_STATUSES

#include "src/components/sync/base/user_selectable_type.cc"
#undef USER_EVENTS
#undef syncer
