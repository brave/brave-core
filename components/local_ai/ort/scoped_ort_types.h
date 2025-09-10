// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_ORT_SCOPED_ORT_TYPES_H_
#define BRAVE_COMPONENTS_LOCAL_AI_ORT_SCOPED_ORT_TYPES_H_

#include <type_traits>

#include "base/scoped_generic.h"
#include "brave/components/local_ai/ort/platform_functions_ort.h"
#include "third_party/onnxruntime_headers/src/include/onnxruntime/core/session/onnxruntime_c_api.h"

namespace local_ai::ort {

namespace internal {

template <typename T>
  requires std::is_pointer<T>::value
struct ScopedOrtTypeTraitsHelper;

template <typename T>
  requires std::is_pointer<T>::value
struct ScopedOrtTypeTraits {
  static T InvalidValue() { return nullptr; }
  static void Free(T value) { ScopedOrtTypeTraitsHelper<T>::Free(value); }
};

template <>
struct ScopedOrtTypeTraitsHelper<OrtEnv*> {
  static void Free(OrtEnv* value) {
    if (auto* platform_functions = PlatformFunctions::GetInstance()) {
      platform_functions->ort_api()->ReleaseEnv(value);
    }
  }
};

template <>
struct ScopedOrtTypeTraitsHelper<OrtSession*> {
  static void Free(OrtSession* value) {
    if (auto* platform_functions = PlatformFunctions::GetInstance()) {
      platform_functions->ort_api()->ReleaseSession(value);
    }
  }
};

template <>
struct ScopedOrtTypeTraitsHelper<OrtSessionOptions*> {
  static void Free(OrtSessionOptions* value) {
    if (auto* platform_functions = PlatformFunctions::GetInstance()) {
      platform_functions->ort_api()->ReleaseSessionOptions(value);
    }
  }
};

template <>
struct ScopedOrtTypeTraitsHelper<OrtStatus*> {
  static void Free(OrtStatus* value) {
    if (auto* platform_functions = PlatformFunctions::GetInstance()) {
      platform_functions->ort_api()->ReleaseStatus(value);
    }
  }
};

template <>
struct ScopedOrtTypeTraitsHelper<OrtValue*> {
  static void Free(OrtValue* value) {
    if (auto* platform_functions = PlatformFunctions::GetInstance()) {
      platform_functions->ort_api()->ReleaseValue(value);
    }
  }
};

template <>
struct ScopedOrtTypeTraitsHelper<OrtMemoryInfo*> {
  static void Free(OrtMemoryInfo* value) {
    if (auto* platform_functions = PlatformFunctions::GetInstance()) {
      platform_functions->ort_api()->ReleaseMemoryInfo(value);
    }
  }
};

template <>
struct ScopedOrtTypeTraitsHelper<OrtOpAttr*> {
  static void Free(OrtOpAttr* value) {
    if (auto* platform_functions = PlatformFunctions::GetInstance()) {
      platform_functions->ort_api()->ReleaseOpAttr(value);
    }
  }
};

template <>
struct ScopedOrtTypeTraitsHelper<OrtTypeInfo*> {
  static void Free(OrtTypeInfo* value) {
    if (auto* platform_functions = PlatformFunctions::GetInstance()) {
      platform_functions->ort_api()->ReleaseTypeInfo(value);
    }
  }
};

template <>
struct ScopedOrtTypeTraitsHelper<OrtTensorTypeAndShapeInfo*> {
  static void Free(OrtTensorTypeAndShapeInfo* value) {
    if (auto* platform_functions = PlatformFunctions::GetInstance()) {
      platform_functions->ort_api()->ReleaseTensorTypeAndShapeInfo(value);
    }
  }
};

template <>
struct ScopedOrtTypeTraitsHelper<OrtValueInfo*> {
  static void Free(OrtValueInfo* value) {
    if (auto* platform_functions = PlatformFunctions::GetInstance()) {
      platform_functions->ort_api()->ReleaseValueInfo(value);
    }
  }
};

template <>
struct ScopedOrtTypeTraitsHelper<OrtNode*> {
  static void Free(OrtNode* value) {
    if (auto* platform_functions = PlatformFunctions::GetInstance()) {
      platform_functions->ort_api()->ReleaseNode(value);
    }
  }
};

template <>
struct ScopedOrtTypeTraitsHelper<OrtGraph*> {
  static void Free(OrtGraph* value) {
    if (auto* platform_functions = PlatformFunctions::GetInstance()) {
      platform_functions->ort_api()->ReleaseGraph(value);
    }
  }
};

template <>
struct ScopedOrtTypeTraitsHelper<OrtModel*> {
  static void Free(OrtModel* value) {
    if (auto* platform_functions = PlatformFunctions::GetInstance()) {
      platform_functions->ort_api()->ReleaseModel(value);
    }
  }
};

template <typename T>
using ScopedOrtType = base::ScopedGeneric<T*, ScopedOrtTypeTraits<T*>>;

}  // namespace internal

using ScopedOrtEnv = internal::ScopedOrtType<OrtEnv>;
using ScopedOrtSession = internal::ScopedOrtType<OrtSession>;
using ScopedOrtSessionOptions = internal::ScopedOrtType<OrtSessionOptions>;
using ScopedOrtStatus = internal::ScopedOrtType<OrtStatus>;
using ScopedOrtValue = internal::ScopedOrtType<OrtValue>;
using ScopedOrtMemoryInfo = internal::ScopedOrtType<OrtMemoryInfo>;
using ScopedOrtOpAttr = internal::ScopedOrtType<OrtOpAttr>;
using ScopedOrtTypeInfo = internal::ScopedOrtType<OrtTypeInfo>;
using ScopedOrtTensorTypeAndShapeInfo =
    internal::ScopedOrtType<OrtTensorTypeAndShapeInfo>;
using ScopedOrtValueInfo = internal::ScopedOrtType<OrtValueInfo>;
using ScopedOrtNode = internal::ScopedOrtType<OrtNode>;
using ScopedOrtGraph = internal::ScopedOrtType<OrtGraph>;
using ScopedOrtModel = internal::ScopedOrtType<OrtModel>;

// Helper function for wrapping OrtStatus* returns that need RAII cleanup
inline ScopedOrtStatus CreateScopedStatus(OrtStatus* status) {
  return ScopedOrtStatus(status);
}

}  // namespace local_ai::ort

#endif  // BRAVE_COMPONENTS_LOCAL_AI_ORT_SCOPED_ORT_TYPES_H_
