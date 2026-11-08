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
  private static let connectionDetent = UISheetPresentationController.Detent.Identifier("connection")
  private let initiallyShowsConnectionSection: Bool
  private var connectionSectionHeight: CGFloat = 0

  public init(
    actions: [Action],
    connectionSection: AnyView? = nil,
    initiallyShowsConnectionSection: Bool = false,
    handlePresentation: @escaping (BrowserMenuPresentation) -> Void
  ) {
    self.initiallyShowsConnectionSection = initiallyShowsConnectionSection && connectionSection != nil
    super.init(
      rootView: BrowserMenu(
        model: .init(
          actions: actions,
          vpnStatus: .liveVPNStatus,
          vpnStatusPublisher: .liveVPNStatus
        ),
        handlePresentation: handlePresentation,
        connectionSection: connectionSection
      )
    )
    rootView.onConnectionSectionHeightChange = { [weak self] height in
      guard let self, self.initiallyShowsConnectionSection else { return }
      // Include the menu's outer top and bottom padding.
      let paddedHeight = height + 32
      guard abs(self.connectionSectionHeight - paddedHeight) > 1 else { return }
      self.connectionSectionHeight = paddedHeight
      let controller = self.sheetPresentationController
        ?? self.popoverPresentationController?.adaptiveSheetPresentationController
      controller?.invalidateDetents()
    }
    rootView.onShowAllActions = { [weak self] in
      guard let self else { return }
      if let controller = sheetPresentationController
        ?? popoverPresentationController?.adaptiveSheetPresentationController
      {
        controller.animateChanges {
          controller.selectedDetentIdentifier = .large
        }
      }
      preferredContentSize.height = view.intrinsicContentSize.height
    }
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  public override func viewDidLoad() {
    super.viewDidLoad()

    view.backgroundColor = .clear
    popoverPresentationController?.backgroundColor = .clear
  }

  public override func viewIsAppearing(_ animated: Bool) {
    super.viewIsAppearing(animated)

    let size = view.intrinsicContentSize
    if initiallyShowsConnectionSection, connectionSectionHeight == 0,
      let section = rootView.connectionSection
    {
      let measurement = UIHostingController(rootView: section.padding())
      connectionSectionHeight = measurement.sizeThatFits(
        in: CGSize(width: view.bounds.width, height: .greatestFiniteMagnitude)
      ).height
    }
    if let controller = sheetPresentationController
      ?? popoverPresentationController?.adaptiveSheetPresentationController
    {
      if initiallyShowsConnectionSection {
        controller.detents = [
          .custom(identifier: Self.connectionDetent) { [weak self] context in
            min(context.maximumDetentValue, self?.connectionSectionHeight ?? size.height)
          },
          .large(),
        ]
        controller.selectedDetentIdentifier = Self.connectionDetent
        controller.prefersScrollingExpandsWhenScrolledToEdge = true
      } else {
        controller.detents = [
          .custom(resolver: { context in
            return min(context.maximumDetentValue * 0.7, size.height)
          }), .large(),
        ]
      }
      controller.prefersGrabberVisible = true
      controller.prefersEdgeAttachedInCompactHeight = true
    }

    preferredContentSize = CGSize(width: 375, height: size.height)
  }
}
