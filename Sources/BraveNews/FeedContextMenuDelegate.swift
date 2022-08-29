// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared

/// Handles context menu related interactions on an item in the feed
class FeedContextMenuDelegate: NSObject, UIContextMenuInteractionDelegate {
  /// The identifier given to context menus created for Brave News feed items
  static let identifier = "feed-item"
  /// Called when a user performs a preview action
  ///
  /// This closure is called at as a completion of the preview action animation
  let performedPreviewAction: () -> Void
  /// The menu items to display.
  ///
  /// This closure is called when a configuration is being requested by the context menu interaction.
  /// Returning `nil` will cause no menu to show up
  let menu: () -> UIMenu?
  /// Whether or not the preview should be padded and include a background
  ///
  /// Set this when an item is included in a group or where the item itself does not contain a background or
  /// its own padding. A dark background is included in the padded preview
  let padPreview: Bool

  init(performedPreviewAction: @escaping () -> Void, menu: @escaping () -> UIMenu?, padPreview: Bool = false) {
    self.performedPreviewAction = performedPreviewAction
    self.menu = menu
    self.padPreview = padPreview
    super.init()
  }

  func contextMenuInteraction(_ interaction: UIContextMenuInteraction, configurationForMenuAtLocation location: CGPoint) -> UIContextMenuConfiguration? {
    guard let menu = self.menu() else { return nil }
    return UIContextMenuConfiguration(
      identifier: Self.identifier as NSString,
      previewProvider: nil,
      actionProvider: { _ in
        return menu
      }
    )
  }

  func contextMenuInteraction(_ interaction: UIContextMenuInteraction, willPerformPreviewActionForMenuWith configuration: UIContextMenuConfiguration, animator: UIContextMenuInteractionCommitAnimating) {
    animator.addCompletion(performedPreviewAction)
  }

  private func targetedPreview(for interaction: UIContextMenuInteraction) -> UITargetedPreview? {
    guard let view = interaction.view else { return nil }
    let preview = UITargetedPreview(view: view)
    if padPreview {
      preview.parameters.backgroundColor = UIColor(white: 0.3, alpha: 1.0)
      preview.parameters.visiblePath = UIBezierPath(roundedRect: view.bounds.insetBy(dx: -10, dy: -10), cornerRadius: 10)
    } else {
      preview.parameters.backgroundColor = .clear
    }
    return preview
  }

  func contextMenuInteraction(_ interaction: UIContextMenuInteraction, previewForHighlightingMenuWithConfiguration configuration: UIContextMenuConfiguration) -> UITargetedPreview? {
    targetedPreview(for: interaction)
  }

  func contextMenuInteraction(_ interaction: UIContextMenuInteraction, previewForDismissingMenuWithConfiguration configuration: UIContextMenuConfiguration) -> UITargetedPreview? {
    targetedPreview(for: interaction)
  }
}
