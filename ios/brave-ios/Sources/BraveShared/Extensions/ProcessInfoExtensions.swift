// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import LocalAuthentication

private let _isiOSAppOnVisionOS: Bool = {
  #if targetEnvironment(simulator)
  return ProcessInfo.processInfo.environment["SIMULATOR_MODEL_IDENTIFIER"]?.hasPrefix(
    "RealityDevice"
  ) ?? false
  #else
  let authContext = LAContext()
  _ = authContext.canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: nil)
  return authContext.biometryType == .opticID
    || NSClassFromString("UIWindowSceneGeometryPreferencesVision") != nil
  #endif
}()

extension ProcessInfo {
  public var isiOSAppOnVisionOS: Bool {
    return _isiOSAppOnVisionOS
  }
}
