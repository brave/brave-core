// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_COMPONENT_UPDATER_BRAVE_COMPONENT_CONTENTS_VERIFIER_H_
#define BRAVE_BROWSER_COMPONENT_UPDATER_BRAVE_COMPONENT_CONTENTS_VERIFIER_H_

#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_component_updater/browser/component_contents_verifier.h"

namespace base {
class FilePath;
}

namespace component_updater {

scoped_refptr<brave_component_updater::ComponentContentsAccessor>
CreateComponentContentsAccessor(bool with_verifier,
                                const base::FilePath& component_root);

void SetupComponentContentsVerifier();

}  // namespace component_updater

#endif  // BRAVE_BROWSER_COMPONENT_UPDATER_BRAVE_COMPONENT_CONTENTS_VERIFIER_H_
