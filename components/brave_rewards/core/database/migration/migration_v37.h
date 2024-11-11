/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_MIGRATION_MIGRATION_V37_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_MIGRATION_MIGRATION_V37_H_

namespace brave_rewards::internal::database::migration {

inline constexpr char v37[] =
    R"(
     CREATE TABLE external_transactions (
       transaction_id TEXT NOT NULL CHECK(transaction_id <> ''),
       contribution_id TEXT NOT NULL CHECK(contribution_id <> ''),
       destination TEXT NOT NULL CHECK(destination <> ''),
       amount TEXT NOT NULL CHECK(amount <> ''),
       created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
       PRIMARY KEY (contribution_id, destination),
       FOREIGN KEY (contribution_id) )"
    R"(REFERENCES contribution_info (contribution_id) )"
    R"(ON UPDATE RESTRICT ON DELETE RESTRICT
     );
   )";

}  // namespace brave_rewards::internal::database::migration

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_MIGRATION_MIGRATION_V37_H_
