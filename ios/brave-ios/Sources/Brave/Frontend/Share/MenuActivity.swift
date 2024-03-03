// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import SwiftUI
import UIKit

protocol MenuActivity: UIActivity {
  /// The image to use when shown on the menu.
  var menuImage: Image { get }
}

/// A standard activity that will appear in the apps menu and executes a callback when the user selects it
class BasicMenuActivity: UIActivity, MenuActivity {
  private let title: String
  private let braveSystemImage: String
  private let activityTypeID: String
  private let callback: () -> Void

  init(
    title: String,
    braveSystemImage: String,
    activityTypeID: String,
    callback: @escaping () -> Void
  ) {
    self.title = title
    self.braveSystemImage = braveSystemImage
    self.activityTypeID = activityTypeID
    self.callback = callback
  }

  // MARK: - UIActivity

  override var activityTitle: String? {
    return title
  }

  override var activityImage: UIImage? {
    return UIImage(braveSystemNamed: braveSystemImage)?.applyingSymbolConfiguration(
      .init(scale: .large)
    )
  }

  override func perform() {
    callback()
    activityDidFinish(true)
  }

  override func canPerform(withActivityItems activityItems: [Any]) -> Bool {
    return true
  }

  override var activityType: UIActivity.ActivityType {
    let bundleId =
      AppInfo.applicationBundle.object(forInfoDictionaryKey: "CFBundleIdentifier") as? String

    return UIActivity.ActivityType(rawValue: "\(bundleId ?? "")\(".\(activityTypeID)")")
  }

  // MARK: - MenuActivity

  var menuImage: Image {
    Image(braveSystemName: braveSystemImage)
  }
}
