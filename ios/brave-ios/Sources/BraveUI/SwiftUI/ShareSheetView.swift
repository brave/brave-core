// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

public struct ShareSheetView: UIViewControllerRepresentable {
  let activityItems: [Any]
  let applicationActivities: [UIActivity]?
  let excludedActivityTypes: [UIActivity.ActivityType]?
  let callback: UIActivityViewController.CompletionWithItemsHandler?

  public init(
    activityItems: [Any],
    applicationActivities: [UIActivity]?,
    excludedActivityTypes: [UIActivity.ActivityType]?,
    callback: UIActivityViewController.CompletionWithItemsHandler?
  ) {
    self.activityItems = activityItems
    self.applicationActivities = applicationActivities
    self.excludedActivityTypes = excludedActivityTypes
    self.callback = callback
  }

  public func makeUIViewController(context: Context) -> UIActivityViewController {
    let controller = UIActivityViewController(
      activityItems: activityItems,
      applicationActivities: applicationActivities
    )
    controller.excludedActivityTypes = excludedActivityTypes
    controller.completionWithItemsHandler = callback
    return controller
  }

  public func updateUIViewController(_ uiViewController: UIActivityViewController, context: Context)
  {
    // nothing to do here
  }
}
