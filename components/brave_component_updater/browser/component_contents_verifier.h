// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_COMPONENT_CONTENTS_VERIFIER_H_
#define BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_COMPONENT_CONTENTS_VERIFIER_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/ref_counted.h"
#include "base/types/expected.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace brave_component_updater {

// Use on MAY_BLOCK sequence.
class ComponentContentsAccessor
    : public base::RefCountedThreadSafe<ComponentContentsAccessor> {
 public:
  enum class Error {
    kFileReadFailed,
    kInvalidSignature,
  };

  virtual const base::FilePath& GetComponentRoot() const = 0;
  virtual bool IsComponentSignatureValid() const = 0;
  virtual void IgnoreInvalidSignature(bool ignore) = 0;
  virtual bool VerifyContents(const base::FilePath& relative_path,
                              base::span<const uint8_t> contents) = 0;

  base::expected<std::string, Error> ReadFileToString(
      const base::FilePath& relative_path);
  base::expected<std::vector<uint8_t>, Error> ReadFileToBytes(
      const base::FilePath& relative_path);

  std::string GetFileAsString(const base::FilePath& relative_path,
                              const std::string& default_value);
  std::vector<uint8_t> GetFileAsBytes(
      const base::FilePath& relative_path,
      const std::vector<uint8_t>& default_value);

 protected:
  friend class base::RefCountedThreadSafe<ComponentContentsAccessor>;
  virtual ~ComponentContentsAccessor() = default;
};

class ComponentContentsVerifier {
 public:
  using ComponentContentsAccessorFactory =
      base::RepeatingCallback<scoped_refptr<ComponentContentsAccessor>(
          const base::FilePath& component_root)>;

  static void Setup(ComponentContentsAccessorFactory cca_factory);
  static ComponentContentsVerifier* GetInstance();

  // Must be called on the MAY_BLOCK sequence.
  scoped_refptr<ComponentContentsAccessor> GetContentsAccessor(
      const base::FilePath& component_root);

 private:
  friend base::NoDestructor<ComponentContentsVerifier>;

  ComponentContentsAccessorFactory cca_factory_;

  ComponentContentsVerifier();
  ~ComponentContentsVerifier();
};

}  // namespace brave_component_updater

#endif  // BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_COMPONENT_CONTENTS_VERIFIER_H_
