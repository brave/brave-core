// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveVPN
import Foundation
import SwiftUI

public enum BrowserMenuPresentation {
  case settings
  case vpnRegionPicker
}

public class BrowserMenuController: UIHostingController<BrowserMenu> {
  public init(
    actions: [Action],
    handlePresentation: @escaping (BrowserMenuPresentation) -> Void
  ) {
    super.init(
      rootView: BrowserMenu(
        model: .init(
          actions: actions,
          vpnStatus: .liveVPNStatus,
          vpnStatusPublisher: .liveVPNStatus
        ),
        handlePresentation: handlePresentation
      )
    )
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  public override func viewDidLoad() {
    super.viewDidLoad()

    view.backgroundColor = .clear
  }

  public override func viewIsAppearing(_ animated: Bool) {
    super.viewIsAppearing(animated)

    let size = view.intrinsicContentSize
    if let controller = sheetPresentationController
      ?? popoverPresentationController?.adaptiveSheetPresentationController
    {
      controller.detents = [
        .custom(resolver: { context in
          return min(context.maximumDetentValue * 0.7, size.height)
        }), .large(),
      ]
      controller.prefersGrabberVisible = true
    }
  }
}
