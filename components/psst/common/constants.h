// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_COMMON_CONSTANTS_H_
#define BRAVE_COMPONENTS_PSST_COMMON_CONSTANTS_H_

namespace psst {

inline constexpr char kUserScriptResultUserPropName[] = "user";
inline constexpr char kUserScriptResultTasksPropName[] = "tasks";
inline constexpr char kUserScriptResultInitialExecutionPropName[] =
    "initial_execution";
inline constexpr char kUserScriptResultSiteNamePropName[] = "name";
inline constexpr char kUserScriptResultShareExpLinkPropName[] =
    "share_experience_link";
inline constexpr char kUserScriptResultTaskItemUrlPropName[] = "url";
inline constexpr char kUserScriptResultTaskItemDescPropName[] = "description";

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_COMMON_CONSTANTS_H_
