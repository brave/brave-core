// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

public final class DeviceOrientation {
    
  private var windowScene: UIWindowScene? {
    return UIApplication.shared.connectedScenes.first as? UIWindowScene
  }
  
  public static let shared: DeviceOrientation = DeviceOrientation()
  
  public func changeOrientation(_ orientationMask: UIInterfaceOrientationMask) {
    if #available(iOS 16.0, *) {
      windowScene?.requestGeometryUpdate(.iOS(interfaceOrientations: orientationMask))
    } else {
      var orientation: UIInterfaceOrientation?
      
      switch orientationMask {
      case .portrait:
        orientation = UIInterfaceOrientation.portrait
      case .portraitUpsideDown:
        orientation = UIInterfaceOrientation.portraitUpsideDown
      case .landscapeRight:
        orientation = UIInterfaceOrientation.landscapeRight
      case .landscapeLeft:
        orientation = UIInterfaceOrientation.landscapeLeft
      default:
        orientation = UIInterfaceOrientation.unknown
      }
      
      if let orientation = orientation {
        UIDevice.current.setValue(orientation.rawValue, forKey: "orientation")
      }
    }
  }
    
  private var isLandscape: Bool {
    if #available(iOS 16.0, *) {
      return windowScene?.interfaceOrientation.isLandscape ?? false
    }
    
    return UIDevice.current.orientation.isLandscape
  }
    
  private var isPortrait: Bool {
    if #available(iOS 16.0, *) {
      return windowScene?.interfaceOrientation.isPortrait ?? false
    }
    return UIDevice.current.orientation.isPortrait
  }
  
  public func changeOrientationToPortraitOnPhone() {
    if UIDevice.current.userInterfaceIdiom != .pad && isLandscape {
      changeOrientation(.portrait)
    }
  }
}

