// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/views/layout/layout_provider.h"

#include <ui/views/layout/layout_provider.cc>

namespace views {

int LayoutProvider::GetCornerRadiusMetric(ShapeContextTokensOverride id) const {
  return 4;
}

}  // namespace views
