// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_COMPONENT_CONTENTS_ACCESSOR_H_
#define BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_COMPONENT_CONTENTS_ACCESSOR_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"

namespace component_updater {

// This class provides secure access to the component's files. It requires
// 'verified_contents.json' that should be shipped with the component. If
// 'verified_contents.json' is missing or signature doesn't match the
// content of the files then accessor doesn't return data from GetFile*
// functions. Use on MAY_BLOCK sequence.
class ComponentContentsAccessor
    : public base::RefCountedThreadSafe<ComponentContentsAccessor> {
 public:
  ComponentContentsAccessor(const ComponentContentsAccessor&) = delete;
  ComponentContentsAccessor(ComponentContentsAccessor&&) = delete;
  ComponentContentsAccessor& operator=(const ComponentContentsAccessor&) =
      delete;
  ComponentContentsAccessor& operator=(ComponentContentsAccessor&&) = delete;

  static scoped_refptr<ComponentContentsAccessor> Create(
      const base::FilePath& component_root);

  const base::FilePath& GetComponentRoot() const;

  std::optional<std::string> GetFileAsString(
      const base::FilePath& relative_path);
  std::optional<std::vector<uint8_t>> GetFileAsBytes(
      const base::FilePath& relative_path);

 private:
  friend class base::RefCountedThreadSafe<ComponentContentsAccessor>;
  explicit ComponentContentsAccessor(const base::FilePath& component_root);
  ~ComponentContentsAccessor();

  const base::FilePath component_root_;
  const std::unique_ptr<const class ContentsVerifier> verifier_;
};

}  // namespace component_updater

#endif  // BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_COMPONENT_CONTENTS_ACCESSOR_H_
