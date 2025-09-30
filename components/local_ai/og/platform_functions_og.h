// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_OG_PLATFORM_FUNCTIONS_OG_H_
#define BRAVE_COMPONENTS_LOCAL_AI_OG_PLATFORM_FUNCTIONS_OG_H_

#include "base/component_export.h"
#include "base/files/file_path.h"
#include "base/no_destructor.h"
#include "base/scoped_native_library.h"
#include "ort_genai_c.h"

namespace local_ai::og {

// Function pointer types for all OGA C API functions
using OgaShutdownProc = decltype(OgaShutdown)*;
using OgaCreateModelProc = decltype(OgaCreateModel)*;
using OgaDestroyModelProc = decltype(OgaDestroyModel)*;
using OgaCreateTokenizerProc = decltype(OgaCreateTokenizer)*;
using OgaDestroyTokenizerProc = decltype(OgaDestroyTokenizer)*;
using OgaCreateTokenizerStreamProc = decltype(OgaCreateTokenizerStream)*;
using OgaDestroyTokenizerStreamProc = decltype(OgaDestroyTokenizerStream)*;
using OgaTokenizerStreamDecodeProc = decltype(OgaTokenizerStreamDecode)*;
using OgaTokenizerApplyChatTemplateProc =
    decltype(OgaTokenizerApplyChatTemplate)*;
using OgaTokenizerEncodeProc = decltype(OgaTokenizerEncode)*;
using OgaCreateSequencesProc = decltype(OgaCreateSequences)*;
using OgaDestroySequencesProc = decltype(OgaDestroySequences)*;
using OgaSequencesGetSequenceCountProc =
    decltype(OgaSequencesGetSequenceCount)*;
using OgaCreateGeneratorParamsProc = decltype(OgaCreateGeneratorParams)*;
using OgaDestroyGeneratorParamsProc = decltype(OgaDestroyGeneratorParams)*;
using OgaGeneratorParamsSetSearchNumberProc =
    decltype(OgaGeneratorParamsSetSearchNumber)*;
using OgaGeneratorParamsSetSearchBoolProc =
    decltype(OgaGeneratorParamsSetSearchBool)*;
using OgaCreateMultiModalProcessorProc =
    decltype(OgaCreateMultiModalProcessor)*;
using OgaDestroyMultiModalProcessorProc =
    decltype(OgaDestroyMultiModalProcessor)*;
using OgaProcessorProcessImagesProc = decltype(OgaProcessorProcessImages)*;
using OgaCreateStringArrayProc = decltype(OgaCreateStringArray)*;
using OgaDestroyStringArrayProc = decltype(OgaDestroyStringArray)*;
using OgaStringArrayAddStringProc = decltype(OgaStringArrayAddString)*;
using OgaLoadImagesProc = decltype(OgaLoadImages)*;
using OgaDestroyImagesProc = decltype(OgaDestroyImages)*;
using OgaDestroyNamedTensorsProc = decltype(OgaDestroyNamedTensors)*;
using OgaCreateGeneratorProc = decltype(OgaCreateGenerator)*;
using OgaDestroyGeneratorProc = decltype(OgaDestroyGenerator)*;
using OgaGenerator_GenerateNextTokenProc =
    decltype(OgaGenerator_GenerateNextToken)*;
using OgaGenerator_IsDoneProc = decltype(OgaGenerator_IsDone)*;
using OgaGenerator_AppendTokenSequencesProc =
    decltype(OgaGenerator_AppendTokenSequences)*;
using OgaGenerator_SetInputsProc = decltype(OgaGenerator_SetInputs)*;
using OgaGenerator_GetSequenceCountProc =
    decltype(OgaGenerator_GetSequenceCount)*;
using OgaGenerator_GetSequenceDataProc =
    decltype(OgaGenerator_GetSequenceData)*;
using OgaResultGetErrorProc = decltype(OgaResultGetError)*;
using OgaDestroyResultProc = decltype(OgaDestroyResult)*;
using OgaDestroyStringProc = decltype(OgaDestroyString)*;

class COMPONENT_EXPORT(LOCAL_AI) PlatformFunctions {
 public:
  PlatformFunctions(const PlatformFunctions&) = delete;
  PlatformFunctions& operator=(const PlatformFunctions&) = delete;

  static PlatformFunctions* GetInstance();
  static PlatformFunctions* GetInstance(const base::FilePath& library_path);

  bool IsInitialized() const { return og_library_.is_valid(); }

  // Function pointers
  OgaShutdownProc OgaShutdownPtr = nullptr;
  OgaCreateModelProc OgaCreateModelPtr = nullptr;
  OgaDestroyModelProc OgaDestroyModelPtr = nullptr;
  OgaCreateTokenizerProc OgaCreateTokenizerPtr = nullptr;
  OgaDestroyTokenizerProc OgaDestroyTokenizerPtr = nullptr;
  OgaCreateTokenizerStreamProc OgaCreateTokenizerStreamPtr = nullptr;
  OgaDestroyTokenizerStreamProc OgaDestroyTokenizerStreamPtr = nullptr;
  OgaTokenizerStreamDecodeProc OgaTokenizerStreamDecodePtr = nullptr;
  OgaTokenizerApplyChatTemplateProc OgaTokenizerApplyChatTemplatePtr = nullptr;
  OgaTokenizerEncodeProc OgaTokenizerEncodePtr = nullptr;
  OgaCreateSequencesProc OgaCreateSequencesPtr = nullptr;
  OgaDestroySequencesProc OgaDestroySequencesPtr = nullptr;
  OgaSequencesGetSequenceCountProc OgaSequencesGetSequenceCountPtr = nullptr;
  OgaCreateGeneratorParamsProc OgaCreateGeneratorParamsPtr = nullptr;
  OgaDestroyGeneratorParamsProc OgaDestroyGeneratorParamsPtr = nullptr;
  OgaGeneratorParamsSetSearchNumberProc OgaGeneratorParamsSetSearchNumberPtr =
      nullptr;
  OgaGeneratorParamsSetSearchBoolProc OgaGeneratorParamsSetSearchBoolPtr =
      nullptr;
  OgaCreateMultiModalProcessorProc OgaCreateMultiModalProcessorPtr = nullptr;
  OgaDestroyMultiModalProcessorProc OgaDestroyMultiModalProcessorPtr = nullptr;
  OgaProcessorProcessImagesProc OgaProcessorProcessImagesPtr = nullptr;
  OgaCreateStringArrayProc OgaCreateStringArrayPtr = nullptr;
  OgaDestroyStringArrayProc OgaDestroyStringArrayPtr = nullptr;
  OgaStringArrayAddStringProc OgaStringArrayAddStringPtr = nullptr;
  OgaLoadImagesProc OgaLoadImagesPtr = nullptr;
  OgaDestroyImagesProc OgaDestroyImagesPtr = nullptr;
  OgaDestroyNamedTensorsProc OgaDestroyNamedTensorsPtr = nullptr;
  OgaCreateGeneratorProc OgaCreateGeneratorPtr = nullptr;
  OgaDestroyGeneratorProc OgaDestroyGeneratorPtr = nullptr;
  OgaGenerator_GenerateNextTokenProc OgaGenerator_GenerateNextTokenPtr =
      nullptr;
  OgaGenerator_IsDoneProc OgaGenerator_IsDonePtr = nullptr;
  OgaGenerator_AppendTokenSequencesProc OgaGenerator_AppendTokenSequencesPtr =
      nullptr;
  OgaGenerator_SetInputsProc OgaGenerator_SetInputsPtr = nullptr;
  OgaGenerator_GetSequenceCountProc OgaGenerator_GetSequenceCountPtr = nullptr;
  OgaGenerator_GetSequenceDataProc OgaGenerator_GetSequenceDataPtr = nullptr;
  OgaResultGetErrorProc OgaResultGetErrorPtr = nullptr;
  OgaDestroyResultProc OgaDestroyResultPtr = nullptr;
  OgaDestroyStringProc OgaDestroyStringPtr = nullptr;

 private:
  friend class base::NoDestructor<PlatformFunctions>;

  PlatformFunctions();
  explicit PlatformFunctions(const base::FilePath& library_path);
  ~PlatformFunctions();

  // Load ONNX Runtime GenAI from the specified path
  bool LoadOnnxRuntimeGenAILibrary(const base::FilePath& library_path);

  base::ScopedNativeLibrary og_library_;
};

}  // namespace local_ai::og

#endif  // BRAVE_COMPONENTS_LOCAL_AI_OG_PLATFORM_FUNCTIONS_OG_H_
