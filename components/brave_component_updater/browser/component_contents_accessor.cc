// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_component_updater/browser/component_contents_accessor.h"

#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "brave/components/brave_component_updater/browser/component_contents_verifier.h"

namespace component_updater {

ComponentContentsAccessor::ComponentContentsAccessor(
    const base::FilePath& component_root)
    : component_root_(component_root),
      verifier_(CreateContentsVerifier(component_root)) {}

ComponentContentsAccessor::~ComponentContentsAccessor() = default;

// static
scoped_refptr<ComponentContentsAccessor> ComponentContentsAccessor::Create(
    const base::FilePath& component_root) {
  return base::WrapRefCounted(new ComponentContentsAccessor(component_root));
}

const base::FilePath& ComponentContentsAccessor::GetComponentRoot() const {
  return component_root_;
}

std::optional<std::string> ComponentContentsAccessor::GetFileAsString(
    const base::FilePath& relative_path) {
  std::string contents;
  if (!base::ReadFileToString(GetComponentRoot().Append(relative_path),
                              &contents)) {
    return std::nullopt;
  }
  if (verifier_ &&
      !verifier_->VerifyContents(relative_path, base::as_byte_span(contents))) {
    return std::nullopt;
  }

  return contents;
}

std::optional<std::vector<uint8_t>> ComponentContentsAccessor::GetFileAsBytes(
    const base::FilePath& relative_path) {
  auto contents =
      base::ReadFileToBytes(GetComponentRoot().Append(relative_path));
  if (!contents) {
    return std::nullopt;
  }
  if (verifier_ && !verifier_->VerifyContents(relative_path,
                                              base::as_byte_span(*contents))) {
    return std::nullopt;
  }
  return std::move(*contents);
}

}  // namespace component_updater
