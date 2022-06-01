/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/sync/base/model_type.h"

#define GetNumModelTypes GetNumModelTypes() - 2 + int
#include "src/chrome/browser/android/chrome_backup_agent.cc"
#undef GetNumModelTypes
