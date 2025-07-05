// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_COMPONENT_CONTENTS_READER_H_
#define BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_COMPONENT_CONTENTS_READER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/files/file_path.h"
#include "base/functional/callback_forward.h"
#include "base/task/task_runner.h"
#include "base/task/task_traits.h"

namespace component_updater {

// This class provides secure access to the component's files. It requires
// 'verified_contents.json' that should be shipped with the component. If
// 'verified_contents.json' is missing or signature doesn't match the
// content of the files then accessor doesn't return data from GetFile*
// functions.
class COMPONENT_EXPORT(BRAVE_COMPONENT_UPDATER) ComponentContentsReader {
 public:
  using OnGetAsString =
      base::OnceCallback<void(std::optional<std::string> data)>;
  using OnGetAsBytes =
      base::OnceCallback<void(std::optional<std::vector<uint8_t>> data)>;

  ComponentContentsReader(const ComponentContentsReader&) = delete;
  ComponentContentsReader(ComponentContentsReader&&) = delete;
  ComponentContentsReader& operator=(const ComponentContentsReader&) = delete;
  ComponentContentsReader& operator=(ComponentContentsReader&&) = delete;
  ~ComponentContentsReader();

  static std::unique_ptr<ComponentContentsReader> Create(
      const base::FilePath& component_root);
  static std::unique_ptr<ComponentContentsReader> CreateBypassForTesting(
      const base::FilePath& component_root);

  // This method is required for now, but will be removed.
  // Please, avoid reading signed component files directly.
  const base::FilePath& GetComponentRootDeprecated() const;

  void GetFileAsString(
      const base::FilePath& relative_path,
      OnGetAsString on_data,
      const base::TaskPriority priority = base::TaskTraits().priority());

  void GetFileAsString(const base::FilePath& relative_path,
                       OnGetAsString on_data,
                       scoped_refptr<base::TaskRunner> file_task_runner);

  void GetFileAsBytes(
      const base::FilePath& relative_path,
      OnGetAsBytes on_data,
      const base::TaskPriority priority = base::TaskTraits().priority());

  void GetFileAsBytes(const base::FilePath& relative_path,
                      OnGetAsBytes on_data,
                      scoped_refptr<base::TaskRunner> file_task_runner);

 private:
  explicit ComponentContentsReader(const base::FilePath& component_root);

  const base::FilePath component_root_;
  const std::unique_ptr<const class ContentsVerifier> verifier_;
};

}  // namespace component_updater

#endif  // BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_COMPONENT_CONTENTS_READER_H_
