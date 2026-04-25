// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import SwiftUI
import UIKit

class WalletPanelActionsController: UIHostingController<WalletActionsView> {
  init(
    handlePanelActionInWebUI: @escaping (WalletActionDestination) -> Void,
    handlePanelActionInNativeUI: @escaping (WalletActionDestination) -> Void
  ) {
    super.init(
      rootView: WalletActionsView(
        action: { destination in
          if FeatureList.kBraveWalletWebUIIOS?.enabled == true {
            handlePanelActionInWebUI(destination)
          } else {
            handlePanelActionInNativeUI(destination)
          }
        }
      )
    )
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  public override func viewDidLoad() {
    super.viewDidLoad()

    view.backgroundColor = .braveBackground
    popoverPresentationController?.backgroundColor = .clear
  }

  public override func viewIsAppearing(_ animated: Bool) {
    super.viewIsAppearing(animated)

    let size = view.intrinsicContentSize
    if let controller = sheetPresentationController
      ?? popoverPresentationController?.adaptiveSheetPresentationController
    {
      controller.detents = [
        .custom(resolver: { context in
          return size.height
        }), .large(),
      ]
      controller.prefersGrabberVisible = false
      controller.prefersEdgeAttachedInCompactHeight = true
    }

    preferredContentSize = CGSize(width: 375, height: size.height)
  }
}
