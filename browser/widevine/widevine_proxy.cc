/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "base/logging.h"
#include "base/scoped_native_library.h"
#include "media/cdm/api/content_decryption_module.h"
#include "media/cdm/api/content_decryption_module_ext.h"

using base::FilePath;
using base::GetFunctionPointerFromNativeLibrary;
using base::StringPiece;

static base::ScopedNativeLibrary g_orig_dll;

extern "C"
CDM_API void SetOrigDll(const FilePath& dll_path) {
  VLOG(1) << "SetOrigDll(" << dll_path << ")";
  g_orig_dll = base::ScopedNativeLibrary(dll_path);
  VLOG(1) << "SetOrigDll completed.";
}

template <typename F>
F GetFunctionPointerFromOrigDll(const char* name) {
  return reinterpret_cast<F>(g_orig_dll.GetFunctionPointer(name));
}

// INITIALIZE_CDM_MODULE is a macro. However, we need to pass it as a string to
// GetFunctionPointer(). The following macros make this possible.
#define STRINGIFY(X) #X
#define MAKE_STRING(X) STRINGIFY(X)

CDM_API void INITIALIZE_CDM_MODULE() {
  VLOG(1) << "INITIALIZE_CDM_MODULE";
  using OrigFunc = void (*)();
  const char* function_name = MAKE_STRING(INITIALIZE_CDM_MODULE);
  GetFunctionPointerFromOrigDll<OrigFunc>(function_name)();
}

CDM_API void DeinitializeCdmModule() {
  VLOG(1) << "DeinitializeCdmModule";
  using OrigFunc = void (*)();
  GetFunctionPointerFromOrigDll<OrigFunc>("DeinitializeCdmModule")();
}

CDM_API void* CreateCdmInstance(int cdm_interface_version,
                                const char* key_system,
                                uint32_t key_system_size,
                                GetCdmHostFunc get_cdm_host_func,
                                void* user_data) {
  VLOG(1) << "CreateCdmInstance";
  using OrigFunc = void* (*)(int, const char*, uint32_t, GetCdmHostFunc, void*);
  return GetFunctionPointerFromOrigDll<OrigFunc>("CreateCdmInstance")(
    cdm_interface_version, key_system, key_system_size, get_cdm_host_func,
    user_data);
}

CDM_API const char* GetCdmVersion() {
  VLOG(1) << "GetCdmVersion";
  using OrigFunc = const char* (*)();
  return GetFunctionPointerFromOrigDll<OrigFunc>("GetCdmVersion")();
}

CDM_API bool VerifyCdmHost_0(const cdm::HostFile* host_files,
                             uint32_t num_files) {
  VLOG(1) << "VerifyCdmHost_0";
  using OrigFunc = bool (*)(const cdm::HostFile*, uint32_t);
  return GetFunctionPointerFromOrigDll<OrigFunc>("VerifyCdmHost_0")(
    host_files, num_files);
}
