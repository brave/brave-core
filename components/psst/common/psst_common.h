// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_COMMON_PSST_COMMON_H_
#define BRAVE_COMPONENTS_PSST_COMMON_PSST_COMMON_H_

namespace psst {

enum class ConsentStatus {
  kAsk,    // show the popup dialog to ask user to apply privacy
  kAllow,  // continue to apply privacy with no prompts
  kBlock   // do not ask user any more
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_COMMON_PSST_COMMON_H_
