// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/ort/platform_functions_ort.h"

#include "base/logging.h"
#include "base/native_library.h"

namespace local_ai::ort {

namespace {

using OrtGetApiBaseProc = decltype(OrtGetApiBase)*;

}  // namespace

PlatformFunctions::PlatformFunctions() {
  // Default constructor - no library loaded
}

PlatformFunctions::PlatformFunctions(const base::FilePath& library_path) {
  if (!LoadOnnxRuntimeLibrary(library_path)) {
    LOG(WARNING) << "[LocalAI] Failed to load ONNX Runtime library from: "
                 << library_path.value();
  }
}

PlatformFunctions::~PlatformFunctions() = default;

// static
PlatformFunctions* PlatformFunctions::GetInstance() {
  static base::NoDestructor<PlatformFunctions> instance;
  return instance.get();
}

// static
PlatformFunctions* PlatformFunctions::GetInstance(
    const base::FilePath& library_path) {
  // Get the same singleton instance and initialize it if needed
  auto* instance = GetInstance();
  if (instance && !instance->IsInitialized()) {
    instance->LoadOnnxRuntimeLibrary(library_path);
  }
  return instance;
}

bool PlatformFunctions::LoadOnnxRuntimeLibrary(
    const base::FilePath& library_path) {
  LOG(INFO) << "[LocalAI] Attempting to load ONNX Runtime from: "
            << library_path.value();

  base::NativeLibraryLoadError error;
  ort_library_.reset(base::LoadNativeLibrary(library_path, &error));

  if (!ort_library_.is_valid()) {
    LOG(ERROR) << "[LocalAI] Failed to load ONNX Runtime library from: "
               << library_path.value() << " Error: " << error.ToString();
    return false;
  }

  LOG(INFO) << "[LocalAI] Successfully loaded ONNX Runtime library: "
            << library_path.value();

  // Get the OrtGetApiBase function
  auto get_api_base = reinterpret_cast<OrtGetApiBaseProc>(
      ort_library_.GetFunctionPointer("OrtGetApiBase"));

  if (!get_api_base) {
    LOG(ERROR) << "[LocalAI] Failed to find OrtGetApiBase function in ONNX "
               << "Runtime library";
    ort_library_.reset();
    return false;
  }

  // Get the API base and then the specific API version
  const OrtApiBase* api_base = get_api_base();
  if (!api_base) {
    LOG(ERROR) << "[LocalAI] OrtGetApiBase returned null pointer";
    ort_library_.reset();
    return false;
  }

  ort_api_ = api_base->GetApi(ORT_API_VERSION);
  if (!ort_api_) {
    LOG(ERROR) << "[LocalAI] Failed to get OrtApi for API Version "
               << ORT_API_VERSION;
    ort_library_.reset();
    return false;
  }

  LOG(INFO) << "[LocalAI] Successfully initialized ONNX Runtime API";
  return true;
}

}  // namespace local_ai::ort
