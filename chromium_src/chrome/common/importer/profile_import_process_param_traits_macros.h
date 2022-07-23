/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// NOLINT(build/header_guard)
// no-include-guard-because-multiply-included

#include "build/build_config.h"
#include "chrome/common/importer/importer_type.h"

#if !BUILDFLAG(IS_WIN)
#define TYPE_BOOKMARKS_FILE TYPE_OPERA
#else
#define TYPE_EDGE TYPE_OPERA
#endif
#include "src/chrome/common/importer/profile_import_process_param_traits_macros.h"
#if !BUILDFLAG(IS_WIN)
#undef TYPE_BOOKMARKS_FILE
#else
#undef TYPE_EDGE
#endif
