// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import LocalAuthentication

extension ProcessInfo {
  public var isiOSAppOnVisionOS: Bool {
    #if targetEnvironment(simulator)
    return environment["SIMULATOR_MODEL_IDENTIFIER"]?.hasPrefix("RealityDevice") ?? false
    #else
    if #available(iOS 17.0, *) {
      // Vision Pro ships with iOS 17.0 so this will always execute
      let authContext = LAContext()
      _ = authContext.canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: nil)
      return authContext.biometryType == .opticID
        || NSClassFromString("UIWindowSceneGeometryPreferencesVision") != nil
    }
    return false
    #endif
  }
}
