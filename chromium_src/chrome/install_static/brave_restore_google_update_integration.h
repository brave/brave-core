/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Include this file to restore BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION
// after stashing its value via brave_stash_google_update_integration.h.

// NOLINT(build/header_guard)
// no-include-guard-because-multiply-included

#undef BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION

#ifdef BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION_WAS_0
#define BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION() (0)
#undef BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION_WAS_0
#endif

#ifdef BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION_WAS_1
#define BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION() (1)
#undef BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION_WAS_1
#endif
