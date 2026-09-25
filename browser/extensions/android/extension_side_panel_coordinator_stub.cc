// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/extensions/extension_side_panel_coordinator.h"

namespace extensions {

// The extension side panel UI is desktop Android only; the coordinator
// supports running without a delegate.
// static
std::unique_ptr<ExtensionSidePanelCoordinator::Delegate>
ExtensionSidePanelCoordinator::CreateDelegate(
    ExtensionSidePanelCoordinator* coordinator) {
  return nullptr;
}

}  // namespace extensions
