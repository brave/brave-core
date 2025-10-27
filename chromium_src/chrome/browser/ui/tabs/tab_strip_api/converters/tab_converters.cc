// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/browser_apis/tab_strip/tab_strip_api.mojom.h"

#define SPLIT \
  SPLIT:      \
  case tabs::TabCollection::Type::TREE_NODE

#include <chrome/browser/ui/tabs/tab_strip_api/converters/tab_converters.cc>

#undef SPLIT
