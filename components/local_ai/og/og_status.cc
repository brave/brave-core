// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/og/og_status.h"

#include "base/strings/strcat.h"
#include "brave/components/local_ai/og/platform_functions_og.h"
#include "ort_genai_c.h"

namespace local_ai::og {

namespace internal {

std::string OgaResultErrorMessage(OgaResult* result) {
  CHECK(result);

  auto* platform_functions = PlatformFunctions::GetInstance();
  if (!platform_functions || !platform_functions->IsInitialized()) {
    return "[LocalAI] ONNX Runtime GenAI not initialized";
  }

  constexpr char kOgaErrorMessage[] = "[LocalAI] OGA error: ";
  const char* error_msg = platform_functions->OgaResultGetErrorPtr(result);
  std::string error_string = base::StrCat({kOgaErrorMessage, error_msg});

  // Clean up the result
  platform_functions->OgaDestroyResultPtr(result);

  return error_string;
}

}  // namespace internal

}  // namespace local_ai::og
