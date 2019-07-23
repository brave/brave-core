/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/common/media/cdm_host_file_path.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/stl_util.h"
#include "build/build_config.h"
#include "chrome/common/chrome_version.h"

#if defined(OS_MACOSX)
#include "base/mac/bundle_locations.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/chrome_constants.h"
#endif

// All above headers copied from original cdm_host_file_path.cc are included
// to prevent below GOOGLE_CHROEM_BUILD affect them.

#define GOOGLE_CHROME_BUILD
#include "../../../../../chrome/common/media/cdm_host_file_path.cc"  // NOLINT
#undef GOOGLE_CHROME_BUILD
