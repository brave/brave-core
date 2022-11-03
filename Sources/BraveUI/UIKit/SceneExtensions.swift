// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

extension UIView {
  /// Returns the `Scene` that this view belongs to.
  /// If the view does not belong to a scene, it returns the scene of its parent
  /// Otherwise returns nil if no scene is associated with this view.
  public var currentScene: UIWindowScene? {
    if let scene = window?.windowScene {
      return scene
    }
    
    if let scene = superview?.currentScene {
      return scene
    }
    
    return nil
  }
}

extension UIViewController {
  /// Returns the `Scene` that this controller belongs to.
  /// If the controller does not belong to a scene, it returns the scene of its presenter or parent.
  /// Otherwise returns nil if no scene is associated with this controller.
  public var currentScene: UIWindowScene? {
    if let scene = view.window?.windowScene {
      return scene
    }
    
    if let scene = parent?.currentScene {
      return scene
    }
    
    if let scene = presentingViewController?.currentScene {
      return scene
    }
    
    return nil
  }
}
