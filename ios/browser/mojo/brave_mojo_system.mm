// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/mojo/brave_mojo_system.h"

#include "mojo/public/c/system/thunks.h"

bool BraveMojoIsInitialized(void) {
  // The thunks table is populated by mojo::core::Init(), before that every
  // mojo system call is fatal.
  return MojoEmbedderGetSystemThunks2()->size > 0;
}
