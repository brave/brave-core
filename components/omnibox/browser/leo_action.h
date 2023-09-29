/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_LEO_ACTION_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_LEO_ACTION_H_

#include <string>

#include "components/omnibox/browser/actions/omnibox_action.h"

class LeoAction : public OmniboxAction {
 public:
  explicit LeoAction(const std::u16string& query);

  // OmniboxAction:
  void Execute(ExecutionContext& context) const override;

 protected:
  ~LeoAction() override;

 private:
  const std::u16string query_;
};

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_LEO_ACTION_H_
