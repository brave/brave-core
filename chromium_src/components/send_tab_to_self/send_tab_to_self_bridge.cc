/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

 #define BRAVE_DISMISS_ENTRY                                               \
   auto entity_data = CopyToEntityData(entry->AsLocalProto().specifics()); \
   change_processor()->Put(guid, std::move(entity_data),                   \
                           batch->GetMetadataChangeList());

#include "../../../../components/send_tab_to_self/send_tab_to_self_bridge.cc"

#undef BRAVE_DISMISS_ENTRY
