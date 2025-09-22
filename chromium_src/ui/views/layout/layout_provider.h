// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_UI_VIEWS_LAYOUT_LAYOUT_PROVIDER_H_
#define BRAVE_CHROMIUM_SRC_UI_VIEWS_LAYOUT_LAYOUT_PROVIDER_H_

namespace views {

// An enum type to provide an override path for kOmniboxExpandedRadius to be
// mapped to ShapeSysTokens::kMedium. This is used by
// `RoundedOmniboxResultsFrame`.
enum class ShapeContextTokensOverride {
  kOmniboxExpandedRadius,
};

}  // namespace views

// This function is added to allow callers to retrieve an overriden value for
// `kOmniboxExpandedRadius` by just replacing `ShapeContextTokens` on the
// caller.
#define GetDialogInsetsForContentType                                \
  Unused();                                                          \
  int GetCornerRadiusMetric(ShapeContextTokensOverride token) const; \
  gfx::Insets GetDialogInsetsForContentType

#include <ui/views/layout/layout_provider.h>  // IWYU pragma: export

#undef GetDialogInsetsForContentType

#endif  // BRAVE_CHROMIUM_SRC_UI_VIEWS_LAYOUT_LAYOUT_PROVIDER_H_
