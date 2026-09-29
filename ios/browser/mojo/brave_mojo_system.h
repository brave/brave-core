// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_MOJO_BRAVE_MOJO_SYSTEM_H_
#define BRAVE_IOS_BROWSER_MOJO_BRAVE_MOJO_SYSTEM_H_

// A minimal interface to the mojo system API exported from BraveCore.framework.
// It exists so that code outside of BraveCore (such as the SwiftMojo
// framework) can talk to C++ mojo services over message pipes without
// statically linking a second copy of mojo.

#import <Foundation/Foundation.h>

// Returns true once mojo has been initialized in this process, after which
// the rest of this API may be used.
OBJC_EXPORT bool BraveMojoIsInitialized(void);

#endif  // BRAVE_IOS_BROWSER_MOJO_BRAVE_MOJO_SYSTEM_H_
