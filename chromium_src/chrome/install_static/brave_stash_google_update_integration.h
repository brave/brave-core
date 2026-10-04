/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// This file is a helper for temporarily changing or setting the value of
// BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION. To use it, include this
// file and define a new value for the flag. Later, you can then restore the
// flag's original value by including brave_restore_google_update_integration.h.

// NOLINT(build/header_guard)
// no-include-guard-because-multiply-included

#ifdef BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION

#if defined(BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION_WAS_0) || \
    defined(BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION_WAS_1)
#error This helper template does not support nesting.
#endif

#if BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION() == 0
#define BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION_WAS_0
#elif BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION() == 1
#define BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION_WAS_1
#else
#error Unexpected value.
#endif

#undef BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION

#endif  // BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION
