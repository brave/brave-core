// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit

public final class DeviceOrientation {

  private var windowScene: UIWindowScene? {
    return UIApplication.shared.connectedScenes.first as? UIWindowScene
  }

  public static let shared: DeviceOrientation = DeviceOrientation()

  public func changeOrientation(_ orientationMask: UIInterfaceOrientationMask) {
    windowScene?.requestGeometryUpdate(.iOS(interfaceOrientations: orientationMask))
  }

  private var isLandscape: Bool {
    return windowScene?.interfaceOrientation.isLandscape ?? false
  }

  private var isPortrait: Bool {
    return windowScene?.interfaceOrientation.isPortrait ?? false
  }

  public func changeOrientationToPortraitOnPhone() {
    if UIDevice.current.userInterfaceIdiom != .pad && isLandscape {
      changeOrientation(.portrait)
    }
  }
}
