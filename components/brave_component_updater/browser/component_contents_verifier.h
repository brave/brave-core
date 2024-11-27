// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_COMPONENT_CONTENTS_VERIFIER_H_
#define BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_COMPONENT_CONTENTS_VERIFIER_H_

#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/ref_counted.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace brave_component_updater {

// Use on MAY_BLOCK sequence.
class ComponentContentsAccessor
    : public base::RefCountedThreadSafe<ComponentContentsAccessor> {
 public:
  explicit ComponentContentsAccessor(const base::FilePath component_root);
  ComponentContentsAccessor(const ComponentContentsAccessor&) = delete;
  ComponentContentsAccessor(ComponentContentsAccessor&&) = delete;
  ComponentContentsAccessor& operator=(const ComponentContentsAccessor&) =
      delete;
  ComponentContentsAccessor& operator=(ComponentContentsAccessor&&) = delete;

  virtual bool IsComponentSignatureValid() const;
  virtual void IgnoreInvalidSignature(bool ignore);
  virtual bool VerifyContents(const base::FilePath& relative_path,
                              base::span<const uint8_t> contents);

  const base::FilePath& GetComponentRoot() const;

  std::optional<std::string> GetFileAsString(
      const base::FilePath& relative_path);
  std::optional<std::vector<uint8_t>> GetFileAsBytes(
      const base::FilePath& relative_path);

 protected:
  friend class base::RefCountedThreadSafe<ComponentContentsAccessor>;
  virtual ~ComponentContentsAccessor() = default;

  const base::FilePath component_root_;
};

class ComponentContentsVerifier {
 public:
  using ComponentContentsAccessorFactory =
      base::RepeatingCallback<scoped_refptr<ComponentContentsAccessor>(
          const base::FilePath& component_root)>;

  static void Setup(ComponentContentsAccessorFactory cca_factory);
  static ComponentContentsVerifier* GetInstance();

  void CreateContentsAccessor(
      const base::FilePath& component_root,
      base::OnceCallback<void(scoped_refptr<ComponentContentsAccessor>)>
          on_created);

 private:
  friend base::NoDestructor<ComponentContentsVerifier>;

  ComponentContentsAccessorFactory cca_factory_;

  ComponentContentsVerifier();
  ~ComponentContentsVerifier();
};

}  // namespace brave_component_updater

#endif  // BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_COMPONENT_CONTENTS_VERIFIER_H_
