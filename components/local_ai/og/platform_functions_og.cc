// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/local_ai/og/platform_functions_og.h"

#include "base/logging.h"
#include "base/native_library.h"

namespace local_ai::og {

PlatformFunctions::PlatformFunctions() {
  // Default constructor - no library loaded
}

PlatformFunctions::PlatformFunctions(const base::FilePath& library_path) {
  if (!LoadOnnxRuntimeGenAILibrary(library_path)) {
    LOG(WARNING) << "[LocalAI] Failed to load ONNX Runtime GenAI library "
                 << "from: " << library_path.value();
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
    instance->LoadOnnxRuntimeGenAILibrary(library_path);
  }
  return instance;
}

bool PlatformFunctions::LoadOnnxRuntimeGenAILibrary(
    const base::FilePath& library_path) {
  LOG(INFO) << "[LocalAI] Attempting to load ONNX Runtime GenAI from: "
            << library_path.value();

  base::NativeLibraryLoadError error;
  og_library_ =
      base::ScopedNativeLibrary(base::LoadNativeLibrary(library_path, &error));

  if (!og_library_.is_valid()) {
    LOG(ERROR) << "[LocalAI] Failed to load ONNX Runtime GenAI library from: "
               << library_path.value() << " Error: " << error.ToString();
    return false;
  }

  LOG(INFO) << "[LocalAI] Successfully loaded ONNX Runtime GenAI library: "
            << library_path.value();

  // Load all function pointers
#define LOAD_FUNCTION(name)                                                \
  name##Ptr =                                                              \
      reinterpret_cast<name##Proc>(og_library_.GetFunctionPointer(#name)); \
  if (!name##Ptr) {                                                        \
    LOG(ERROR) << "[LocalAI] Failed to find " #name " function in ONNX "   \
               << "Runtime GenAI library";                                 \
    og_library_.reset();                                                   \
    return false;                                                          \
  }

  LOAD_FUNCTION(OgaShutdown);
  LOAD_FUNCTION(OgaCreateModel);
  LOAD_FUNCTION(OgaDestroyModel);
  LOAD_FUNCTION(OgaCreateTokenizer);
  LOAD_FUNCTION(OgaDestroyTokenizer);
  LOAD_FUNCTION(OgaCreateTokenizerStream);
  LOAD_FUNCTION(OgaDestroyTokenizerStream);
  LOAD_FUNCTION(OgaTokenizerStreamDecode);
  LOAD_FUNCTION(OgaTokenizerApplyChatTemplate);
  LOAD_FUNCTION(OgaTokenizerEncode);
  LOAD_FUNCTION(OgaCreateSequences);
  LOAD_FUNCTION(OgaDestroySequences);
  LOAD_FUNCTION(OgaSequencesGetSequenceCount);
  LOAD_FUNCTION(OgaCreateGeneratorParams);
  LOAD_FUNCTION(OgaDestroyGeneratorParams);
  LOAD_FUNCTION(OgaGeneratorParamsSetSearchNumber);
  LOAD_FUNCTION(OgaGeneratorParamsSetSearchBool);
  LOAD_FUNCTION(OgaCreateMultiModalProcessor);
  LOAD_FUNCTION(OgaDestroyMultiModalProcessor);
  LOAD_FUNCTION(OgaProcessorProcessImages);
  LOAD_FUNCTION(OgaCreateStringArray);
  LOAD_FUNCTION(OgaDestroyStringArray);
  LOAD_FUNCTION(OgaStringArrayAddString);
  LOAD_FUNCTION(OgaLoadImages);
  LOAD_FUNCTION(OgaDestroyImages);
  LOAD_FUNCTION(OgaDestroyNamedTensors);
  LOAD_FUNCTION(OgaCreateGenerator);
  LOAD_FUNCTION(OgaDestroyGenerator);
  LOAD_FUNCTION(OgaGenerator_GenerateNextToken);
  LOAD_FUNCTION(OgaGenerator_IsDone);
  LOAD_FUNCTION(OgaGenerator_AppendTokenSequences);
  LOAD_FUNCTION(OgaGenerator_SetInputs);
  LOAD_FUNCTION(OgaGenerator_GetSequenceCount);
  LOAD_FUNCTION(OgaGenerator_GetSequenceData);
  LOAD_FUNCTION(OgaResultGetError);
  LOAD_FUNCTION(OgaDestroyResult);
  LOAD_FUNCTION(OgaDestroyString);

#undef LOAD_FUNCTION

  LOG(INFO) << "[LocalAI] Successfully initialized ONNX Runtime GenAI API";
  return true;
}

}  // namespace local_ai::og
