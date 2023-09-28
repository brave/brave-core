/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// NOLINT(build/header_guard)
// no-include-guard-because-multiply-included

#ifdef BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION
#if BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION() == 0
#define BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION_WAS_0
#elif BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION() == 1
#define BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION_WAS_1
#else
#error Unexpected value
#endif
#undef BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION
#endif

#define BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION() (1)
