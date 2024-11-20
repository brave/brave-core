// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_component_updater/browser/component_contents_verifier.h"

#include <utility>
#include <vector>

#include "base/check_is_test.h"
#include "base/files/file_util.h"
#include "base/no_destructor.h"
#include "extensions/buildflags/buildflags.h"

namespace brave_component_updater {

base::expected<std::string, ComponentContentsAccessor::Error>
ComponentContentsAccessor::ReadFileToString(
    const base::FilePath& relative_path) {
  std::string contents;
  if (!base::ReadFileToString(GetComponentRoot().Append(relative_path),
                              &contents)) {
    return base::unexpected(Error::kFileReadFailed);
  }
  if (!VerifyContents(relative_path, base::as_byte_span(contents))) {
    return base::unexpected(Error::kInvalidSignature);
  }

  return base::ok(std::move(contents));
}

base::expected<std::vector<uint8_t>, ComponentContentsAccessor::Error>
ComponentContentsAccessor::ReadFileToBytes(
    const base::FilePath& relative_path) {
  auto contents =
      base::ReadFileToBytes(GetComponentRoot().Append(relative_path));
  if (!contents) {
    return base::unexpected(Error::kFileReadFailed);
  }
  if (!VerifyContents(relative_path, base::as_byte_span(*contents))) {
    return base::unexpected(Error::kInvalidSignature);
  }

  return base::ok(*std::move(contents));
}

std::string ComponentContentsAccessor::GetFileAsString(
    const base::FilePath& relative_path,
    const std::string& default_value) {
  return ReadFileToString(relative_path).value_or(default_value);
}

std::vector<uint8_t> ComponentContentsAccessor::GetFileAsBytes(
    const base::FilePath& relative_path,
    const std::vector<uint8_t>& default_value) {
  return ReadFileToBytes(relative_path).value_or(default_value);
}

ComponentContentsVerifier::ComponentContentsVerifier() = default;
ComponentContentsVerifier::~ComponentContentsVerifier() = default;

// static
void ComponentContentsVerifier::Setup(
    ComponentContentsVerifier::ComponentContentsAccessorFactory cca_factory) {
  if (GetInstance()->cca_factory_) {
    CHECK_IS_TEST();
  }
  GetInstance()->cca_factory_ = std::move(cca_factory);
}

// static
ComponentContentsVerifier* ComponentContentsVerifier::GetInstance() {
  static base::NoDestructor<ComponentContentsVerifier> instance;
  return instance.get();
}

// Must be called on the MAY_BLOCK sequence.
scoped_refptr<ComponentContentsAccessor>
ComponentContentsVerifier::GetContentsAccessor(
    const base::FilePath& component_root) {
  CHECK(cca_factory_);
  return cca_factory_.Run(component_root);
}

}  // namespace brave_component_updater
