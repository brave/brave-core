// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/ort/ort_status.h"

#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/local_ai/ort/platform_functions_ort.h"
#include "third_party/onnxruntime_headers/src/include/onnxruntime/core/session/onnxruntime_c_api.h"

namespace local_ai::ort {

namespace internal {

std::string OrtStatusErrorMessage(OrtStatus* status) {
  CHECK(status);

  auto* platform_functions = PlatformFunctions::GetInstance();
  if (!platform_functions || !platform_functions->IsInitialized()) {
    return "[LocalAI] ONNX Runtime not initialized";
  }

  const OrtApi* ort_api = platform_functions->ort_api();
  constexpr char kOrtErrorCode[] = "[LocalAI] ORT status error code: ";
  constexpr char kOrtErrorMessage[] = " error message: ";

  return base::StrCat(
      {kOrtErrorCode,
       base::NumberToString(static_cast<int>(ort_api->GetErrorCode(status))),
       kOrtErrorMessage, ort_api->GetErrorMessage(status)});
}

}  // namespace internal

}  // namespace local_ai::ort
