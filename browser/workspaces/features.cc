/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/workspaces/features.h"

namespace features {

// Allows saving open tabs and windows as a named workspace for later restore.
// NOTE: Being implemented in phases.
// See https://github.com/brave/brave-browser/issues/54738
BASE_FEATURE(kWorkspaces, base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace features
