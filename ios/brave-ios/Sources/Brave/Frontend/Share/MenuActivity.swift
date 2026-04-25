// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import SwiftUI
import UIKit

protocol MenuActivity: UIActivity {
  var id: String { get }
  /// The image to use when shown on the menu.
  var menuImage: Image { get }
}

/// A standard activity that will appear in the apps menu and executes a callback when the user selects it
class BasicMenuActivity: UIActivity, MenuActivity {
  struct ActivityType {
    var id: String
    var title: String
    var braveSystemImage: String
  }

  private(set) var id: String
  private var title: String
  private var braveSystemImage: String
  private let callback: () -> Bool

  init(
    id: String,
    title: String,
    braveSystemImage: String,
    callback: @escaping () -> Bool
  ) {
    self.id = id
    self.title = title
    self.braveSystemImage = braveSystemImage
    self.callback = callback
  }

  convenience init(
    activityType: ActivityType,
    callback: @escaping () -> Void
  ) {
    self.init(
      id: activityType.id,
      title: activityType.title,
      braveSystemImage: activityType.braveSystemImage,
      callback: {
        callback()
        return true
      }
    )
  }

  // MARK: - UIActivity

  override var activityTitle: String? {
    return title
  }

  override var activityImage: UIImage? {
    guard
      let image = UIImage(braveSystemNamed: braveSystemImage)?
        .applyingSymbolConfiguration(.init(scale: .large))
    else {
      return nil
    }
    let longest = max(image.size.width, image.size.height)
    let size = CGSize(width: longest, height: longest)
    let renderer = UIGraphicsImageRenderer(size: size)
    return renderer.image { _ in
      image.draw(
        at: .init(
          x: (size.width - image.size.width) / 2,
          y: (size.height - image.size.height) / 2
        )
      )
    }
  }

  override func perform() {
    activityDidFinish(callback())
  }

  override func canPerform(withActivityItems activityItems: [Any]) -> Bool {
    return true
  }

  override var activityType: UIActivity.ActivityType {
    return UIActivity.ActivityType(rawValue: id)
  }

  // MARK: - MenuActivity

  var menuImage: Image {
    Image(braveSystemName: braveSystemImage)
  }
}
