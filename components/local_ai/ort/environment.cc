// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/ort/environment.h"

#include "base/logging.h"
#include "base/strings/strcat.h"
#include "brave/components/local_ai/ort/platform_functions_ort.h"

namespace local_ai::ort {

// static
base::expected<scoped_refptr<Environment>, std::string> Environment::Create(
    OrtLoggingLevel logging_level) {
  auto* platform_functions = PlatformFunctions::GetInstance();
  if (!platform_functions || !platform_functions->IsInitialized()) {
    return base::unexpected("ONNX Runtime platform functions not initialized");
  }

  const OrtApi* ort_api = platform_functions->ort_api();

  OrtEnv* env_ptr = nullptr;
  OrtStatus* status = ort_api->CreateEnv(logging_level, "LocalAI", &env_ptr);
  if (status) {
    std::string error_message =
        base::StrCat({"Failed to create ONNX Runtime environment: ",
                      ort_api->GetErrorMessage(status)});
    ort_api->ReleaseStatus(status);
    return base::unexpected(error_message);
  }

  ScopedOrtEnv scoped_env(env_ptr);
  if (!scoped_env.is_valid()) {
    return base::unexpected("Failed to create valid ONNX Runtime environment");
  }

  return base::MakeRefCounted<Environment>(base::PassKey<Environment>(),
                                           std::move(scoped_env));
}

Environment::Environment(base::PassKey<Environment> pass_key, ScopedOrtEnv env)
    : env_(std::move(env)) {
  DCHECK(env_.is_valid());
  LOG(INFO) << "[LocalAI] Created ONNX Runtime environment";
}

Environment::~Environment() {
  LOG(INFO) << "[LocalAI] Destroying ONNX Runtime environment";
}

}  // namespace local_ai::ort
