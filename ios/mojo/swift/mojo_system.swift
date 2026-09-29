// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

internal import BraveMojoSystem

/// Process wide state of the mojo system SwiftMojo communicates over.
public enum MojoSystem {
  /// Whether mojo has been initialized in this process. No pipes can be
  /// created or used before this is true.
  public static var isInitialized: Bool {
    BraveMojoIsInitialized()
  }
}
