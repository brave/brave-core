// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import SwiftUI

protocol MenuActivity: UIActivity {
  /// The image to use when shown on the menu.
  var menuImage: Image { get }
}

/// A standard activity that will appear in the apps menu and executes a callback when the user selects it
class BasicMenuActivity: UIActivity, MenuActivity {
  private let title: String
  private let braveSystemImage: String
  private let callback: () -> Bool
  
  init(
    title: String,
    braveSystemImage: String,
    callback: @escaping () -> Bool
  ) {
    self.title = title
    self.braveSystemImage = braveSystemImage
    self.callback = callback
  }
  
  convenience init(
    title: String,
    braveSystemImage: String,
    callback: @escaping () -> Void
  ) {
    self.init(title: title, braveSystemImage: braveSystemImage, callback: {
      callback()
      return true
    })
  }
  
  // MARK: - UIActivity
  
  override var activityTitle: String? {
    return title
  }
  
  override var activityImage: UIImage? {
    return UIImage(braveSystemNamed: braveSystemImage)?.applyingSymbolConfiguration(.init(scale: .large))
  }
  
  override func perform() {
    activityDidFinish(callback())
  }
  
  override func canPerform(withActivityItems activityItems: [Any]) -> Bool {
    return true
  }
  
  // MARK: - MenuActivity
  
  var menuImage: Image {
    Image(braveSystemName: braveSystemImage)
  }
}
