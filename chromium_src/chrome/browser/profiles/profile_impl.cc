/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/brave_bookmark_model_loaded_observer.h"
#include "brave/browser/profiles/brave_profile_impl.h"
#define BookmarkModelLoadedObserver BraveBookmarkModelLoadedObserver
#define PermissionManagerFactory BravePermissionManagerFactory
#include "../../../../../chrome/browser/profiles/profile_impl.cc"
#undef PermissionManagerFactory
#undef BookmarkModelLoadedObserver
