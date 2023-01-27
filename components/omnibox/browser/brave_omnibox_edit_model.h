// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_OMNIBOX_EDIT_MODEL_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_OMNIBOX_EDIT_MODEL_H_

#include <string>

#include "components/omnibox/browser/omnibox_edit_model.h"

class BraveOmniboxEditModel : public OmniboxEditModel {
 public:
  using OmniboxEditModel::OmniboxEditModel;
  ~BraveOmniboxEditModel() override;

  bool CanPasteAndGo(const std::u16string& text) const override;
};

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_OMNIBOX_EDIT_MODEL_H_
