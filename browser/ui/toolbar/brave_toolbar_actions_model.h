/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TOOLBAR_BRAVE_TOOLBAR_ACTIONS_MODEL_H_
#define BRAVE_BROWSER_UI_TOOLBAR_BRAVE_TOOLBAR_ACTIONS_MODEL_H_

#include "chrome/browser/ui/toolbar/toolbar_actions_model.h"

namespace extensions {
class Extension;
}

// The purposes of this subclass are to:
// - Hide the Brave 'extension' item from the |ToolbarActionsBar|, since it is
//   displayed in the |BraveActionsContainer|
class BraveToolbarActionsModel : public ToolbarActionsModel {
 public:
  using ToolbarActionsModel::ToolbarActionsModel;
  BraveToolbarActionsModel(const BraveToolbarActionsModel&) = delete;
  BraveToolbarActionsModel& operator=(const BraveToolbarActionsModel&) = delete;
  bool ShouldAddExtension(const extensions::Extension* extension) override;
};

#endif  // BRAVE_BROWSER_UI_TOOLBAR_BRAVE_TOOLBAR_ACTIONS_MODEL_H_
