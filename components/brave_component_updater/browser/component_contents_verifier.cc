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
#include "base/task/thread_pool.h"

namespace brave_component_updater {

std::optional<std::string> ComponentContentsAccessor::GetFileAsString(
    const base::FilePath& relative_path) {
  std::string contents;
  if (!base::ReadFileToString(GetComponentRoot().Append(relative_path),
                              &contents)) {
    return std::nullopt;
  }
  if (!VerifyContents(relative_path, base::as_byte_span(contents))) {
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
  if (!VerifyContents(relative_path, base::as_byte_span(*contents))) {
    return std::nullopt;
  }
  return std::move(*contents);
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

void ComponentContentsVerifier::CreateContentsAccessor(
    const base::FilePath& component_root,
    base::OnceCallback<void(scoped_refptr<ComponentContentsAccessor>)>
        on_created) {
  CHECK(cca_factory_);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(cca_factory_, component_root), std::move(on_created));
}

}  // namespace brave_component_updater
