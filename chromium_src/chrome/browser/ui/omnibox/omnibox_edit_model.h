// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_EDIT_MODEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_EDIT_MODEL_H_

#include "chrome/browser/ui/omnibox/omnibox_view.h"

#define CanPasteAndGo                                       \
  CanPasteAndGo_Chromium(const std::u16string& text) const; \
  bool CanPasteAndGo

#define PasteAndGo                                                \
  PasteAndGo_Chromium(const std::u16string& text,                 \
                      base::TimeTicks match_selection_timestamp); \
  void PasteAndGo

#define GetSuperGIcon(...)                 \
  GetSuperGIcon_Unused(__VA_ARGS__) const; \
  ui::ImageModel GetSuperGIcon(__VA_ARGS__)

#include <chrome/browser/ui/omnibox/omnibox_edit_model.h>  // IWYU pragma: export
#undef GetSuperGIcon
#undef CanPasteAndGo
#undef PasteAndGo

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_EDIT_MODEL_H_
