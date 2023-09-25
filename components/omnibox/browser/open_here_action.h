// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_OPEN_HERE_ACTION_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_OPEN_HERE_ACTION_H_

#include "components/omnibox/browser/actions/omnibox_action.h"
#include "components/omnibox/browser/actions/omnibox_action_concepts.h"
#include "url/gurl.h"

class OpenHereAction : public OmniboxAction {
 public:
  explicit OpenHereAction(GURL url);

  void Execute(ExecutionContext& context) const override;

#if defined(SUPPORT_PEDALS_VECTOR_ICONS)
  const gfx::VectorIcon& GetVectorIcon() const override;
#endif

  OmniboxActionId ActionId() const override;

 protected:
  ~OpenHereAction() override;
};

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_OPEN_HERE_ACTION_H_
