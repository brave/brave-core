// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_COMPONENT_CONTENTS_VERIFIER_H_
#define BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_COMPONENT_CONTENTS_VERIFIER_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/functional/callback.h"

namespace component_updater {

class ContentsVerifier {
 public:
  virtual ~ContentsVerifier() = default;
  virtual bool VerifyContents(const base::FilePath& relative_path,
                              base::span<const uint8_t> contents) = 0;
};

using ContentsVerifierFactory =
    base::RepeatingCallback<std::unique_ptr<ContentsVerifier>(
        const base::FilePath& component_root)>;

void SetContentsVerifierFactory(ContentsVerifierFactory factory);

// Uses factory set above to create the verifier.
std::unique_ptr<ContentsVerifier> CreateContentsVerifier(
    const base::FilePath& component_root);

}  // namespace component_updater

#endif  // BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_COMPONENT_CONTENTS_VERIFIER_H_
