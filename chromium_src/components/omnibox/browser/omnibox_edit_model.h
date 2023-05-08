// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_EDIT_MODEL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_EDIT_MODEL_H_

#include "components/omnibox/browser/omnibox_view.h"

#define CanPasteAndGo                                       \
  CanPasteAndGo_Chromium(const std::u16string& text) const; \
  bool CanPasteAndGo
#include "src/components/omnibox/browser/omnibox_edit_model.h"  // IWYU pragma: export
#undef CanPasteAndGo

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_EDIT_MODEL_H_
