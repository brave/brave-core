// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared

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
                let openActions: [UIAction] = [
                    self.openInNewTab,
                    // Brave Today is only available in normal tabs, so this isn't technically required
                    // but good to be on the safe side
                    !PrivateBrowsingManager.shared.isPrivateBrowsing ?
                        self.openInNewPrivateTab :
                        nil
                ].compactMap({ $0 })
                let manageActions = [
                    self.hideContent,
                    self.blockSource
                ]
                
                return UIMenu(title: "", children: [
                    UIMenu(title: "", options: [.displayInline], children: openActions),
                    UIMenu(title: "", options: [.displayInline], children: manageActions)
                ])
            }
        )
    }
    
    func contextMenuInteraction(_ interaction: UIContextMenuInteraction, willPerformPreviewActionForMenuWith configuration: UIContextMenuConfiguration, animator: UIContextMenuInteractionCommitAnimating) {
        animator.addCompletion {
            self.handler(.opened())
        }
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

// MARK: - Actions
@available(iOS 13.0, *)
extension FeedContextMenu {
    private var openInNewTab: UIAction {
        .init(title: Strings.openNewTabButtonTitle, handler: UIAction.deferredActionHandler { _ in
            self.handler(.opened(inNewTab: true))
        })
    }
    
    private var openInNewPrivateTab: UIAction {
        .init(title: Strings.openNewPrivateTabButtonTitle, handler: UIAction.deferredActionHandler { _ in
            self.handler(.opened(inNewTab: true, switchingToPrivateMode: true))
        })
    }
    
    private var hideContent: UIAction {
        // FIXME: Localize, Get our own image
        .init(title: "Hide Content", image: UIImage(systemName: "eye.slash.fill"), handler: UIAction.deferredActionHandler { _ in
            self.handler(.hide)
        })
    }
    
    private var blockSource: UIAction {
        // FIXME: Localize, Get our own image
        .init(title: "Block Source", image: UIImage(systemName: "nosign"), attributes: .destructive, handler: UIAction.deferredActionHandler { _ in
            self.handler(.blockSource)
        })
    }
}
