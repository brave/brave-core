// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_ORT_PLATFORM_FUNCTIONS_ORT_H_
#define BRAVE_COMPONENTS_LOCAL_AI_ORT_PLATFORM_FUNCTIONS_ORT_H_

#include <optional>
#include <vector>

#include "base/component_export.h"
#include "base/files/file_path.h"
#include "base/no_destructor.h"
#include "base/scoped_native_library.h"
#include "third_party/onnxruntime_headers/src/include/onnxruntime/core/session/onnxruntime_c_api.h"

namespace local_ai::ort {

class COMPONENT_EXPORT(LOCAL_AI) PlatformFunctions {
 public:
  PlatformFunctions(const PlatformFunctions&) = delete;
  PlatformFunctions& operator=(const PlatformFunctions&) = delete;

  static PlatformFunctions* GetInstance();
  static PlatformFunctions* GetInstance(const base::FilePath& library_path);

  const OrtApi* ort_api() const { return ort_api_; }

  bool IsInitialized() const { return ort_api_ != nullptr; }

 private:
  friend class base::NoDestructor<PlatformFunctions>;

  PlatformFunctions();
  explicit PlatformFunctions(const base::FilePath& library_path);
  ~PlatformFunctions();

  // Load ONNX Runtime from the specified path
  bool LoadOnnxRuntimeLibrary(const base::FilePath& library_path);

  base::ScopedNativeLibrary ort_library_;
  raw_ptr<const OrtApi> ort_api_ = nullptr;
};

}  // namespace local_ai::ort

#endif  // BRAVE_COMPONENTS_LOCAL_AI_ORT_PLATFORM_FUNCTIONS_ORT_H_
