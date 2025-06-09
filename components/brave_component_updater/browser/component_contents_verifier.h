// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_COMPONENT_CONTENTS_VERIFIER_H_
#define BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_COMPONENT_CONTENTS_VERIFIER_H_

#include <memory>

#include "base/component_export.h"
#include "base/files/file_path.h"
#include "base/functional/callback.h"

namespace component_updater {

// Use on MAY_BLOCK.
class COMPONENT_EXPORT(BRAVE_COMPONENT_UPDATER) ContentChecker {
 public:
  virtual ~ContentChecker() = default;
  virtual bool VerifyContents(base::span<const uint8_t> contents) const = 0;
};

class COMPONENT_EXPORT(BRAVE_COMPONENT_UPDATER) ContentsVerifier {
 public:
  virtual ~ContentsVerifier() = default;
  virtual bool IsValid() const = 0;
  virtual std::unique_ptr<ContentChecker> CreateContentChecker(
      const base::FilePath& relative_path) const = 0;
};

using ContentsVerifierFactory =
    base::RepeatingCallback<std::unique_ptr<ContentsVerifier>(
        const base::FilePath& component_root)>;

COMPONENT_EXPORT(BRAVE_COMPONENT_UPDATER)
void SetContentsVerifierFactory(ContentsVerifierFactory factory);

// Uses factory set above to create the verifier.
std::unique_ptr<ContentsVerifier> CreateContentsVerifier(
    const base::FilePath& component_root);

}  // namespace component_updater

#endif  // BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_COMPONENT_CONTENTS_VERIFIER_H_
