// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_ORT_ENVIRONMENT_H_
#define BRAVE_COMPONENTS_LOCAL_AI_ORT_ENVIRONMENT_H_

#include <string>

#include "base/component_export.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/types/expected.h"
#include "base/types/pass_key.h"
#include "brave/components/local_ai/ort/scoped_ort_types.h"
#include "third_party/onnxruntime_headers/src/include/onnxruntime/core/session/onnxruntime_c_api.h"

namespace local_ai::ort {

// A wrapper of `OrtEnv` which is thread-safe and can be shared across sessions.
// It should be kept alive until all sessions using it are destroyed.
class COMPONENT_EXPORT(LOCAL_AI) Environment
    : public base::RefCountedThreadSafe<Environment> {
 public:
  // Create an ONNX Runtime environment with the specified logging level
  static base::expected<scoped_refptr<Environment>, std::string> Create(
      OrtLoggingLevel logging_level = ORT_LOGGING_LEVEL_WARNING);

  Environment(base::PassKey<Environment> pass_key, ScopedOrtEnv env);
  Environment(const Environment&) = delete;
  Environment& operator=(const Environment&) = delete;

  const OrtEnv* get() const { return env_.get(); }

  // Check if the environment is valid and ready to use
  bool IsValid() const { return env_.is_valid(); }

 private:
  friend class base::RefCountedThreadSafe<Environment>;

  ~Environment();

  ScopedOrtEnv env_;
};

}  // namespace local_ai::ort

#endif  // BRAVE_COMPONENTS_LOCAL_AI_ORT_ENVIRONMENT_H_
