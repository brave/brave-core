// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

@available(iOS 13.0, *)
class FeedContextMenu: NSObject, UIContextMenuInteractionDelegate {
    let handler: (FeedItemAction) -> Void
    let padPreview: Bool
    
    init(handler: @escaping (FeedItemAction) -> Void, padPreview: Bool = false) {
        self.handler = handler
        self.padPreview = padPreview
        super.init()
    }
    
    func contextMenuInteraction(_ interaction: UIContextMenuInteraction, configurationForMenuAtLocation location: CGPoint) -> UIContextMenuConfiguration? {
        return UIContextMenuConfiguration(
            identifier: "feed-item" as NSString,
            previewProvider: nil,
            actionProvider: { _ in
                return UIMenu(title: "", image: nil, identifier: nil, options: [], children: [
                    UIAction(title: "Open in New Tab", handler: { _ in
                        
                    }),
                ])
            }
        )
    }
    
    func contextMenuInteraction(_ interaction: UIContextMenuInteraction, willPerformPreviewActionForMenuWith configuration: UIContextMenuConfiguration, animator: UIContextMenuInteractionCommitAnimating) {
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
            self.handler(.tapped)
        }
    }
    
    private func targetedPreview(for interaction: UIContextMenuInteraction) -> UITargetedPreview? {
        guard let view = interaction.view else { return nil }
        let preview = UITargetedPreview(view: view)
        if padPreview {
            preview.parameters.backgroundColor = UIColor(white: 0.2, alpha: 1.0)
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
