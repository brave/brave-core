/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_PREF_NAMES_H_
#define BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_PREF_NAMES_H_

namespace ephemeral_storage {

// Stores origins to perform a storage cleanup on browser restart.
inline constexpr char kFirstPartyStorageOriginsToCleanup[] =
    "ephemeral_storage.first_party_storage_origins_to_cleanup";

}  // namespace ephemeral_storage

#endif  // BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_PREF_NAMES_H_
