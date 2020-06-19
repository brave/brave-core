/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#if !defined(OS_ANDROID)
#include "brave/utility/importer/brave_profile_import_impl.h"

namespace {

#if !defined(OS_ANDROID)
auto RunBraveProfileImporter(
    mojo::PendingReceiver<brave::mojom::ProfileImport> receiver) {
  return std::make_unique<BraveProfileImportImpl>(std::move(receiver));
}
#endif

}  // namespace

#endif

#if defined(OS_ANDROID)
#define BRAVE_PROFILE_IMPORTER
#else
#define BRAVE_PROFILE_IMPORTER \
    RunBraveProfileImporter,
#endif

#define BRAVE_GET_MAIN_THREAD_SERVICE_FACTORY \
    BRAVE_PROFILE_IMPORTER

#include "../../../../chrome/utility/services.cc"

#undef BRAVE_GET_MAIN_THREAD_SERVICE_FACTORY
#undef BRAVE_PROFILE_IMPORTER
