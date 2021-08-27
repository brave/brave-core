// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

// MARK: - Reordering of cells

extension PlaylistListViewController: UITableViewDragDelegate, UITableViewDropDelegate {
    func tableView(_ tableView: UITableView, canMoveRowAt indexPath: IndexPath) -> Bool {
        true
    }
    
    func tableView(_ tableView: UITableView, editingStyleForRowAt indexPath: IndexPath) -> UITableViewCell.EditingStyle {
        .none
    }
    
    func tableView(_ tableView: UITableView, shouldIndentWhileEditingRowAt indexPath: IndexPath) -> Bool {
        false
    }
    
    func tableView(_ tableView: UITableView, moveRowAt sourceIndexPath: IndexPath, to destinationIndexPath: IndexPath) {
        PlaylistManager.shared.reorderItems(from: sourceIndexPath, to: destinationIndexPath) {
            PlaylistManager.shared.reloadData()
        }
    }
    
    func tableView(_ tableView: UITableView, itemsForBeginning session: UIDragSession, at indexPath: IndexPath) -> [UIDragItem] {
        let item = PlaylistManager.shared.itemAtIndex(indexPath.row)
        let dragItem = UIDragItem(itemProvider: NSItemProvider())
        dragItem.localObject = item
        return [dragItem]
    }
    
    func tableView(_ tableView: UITableView, dropSessionDidUpdate session: UIDropSession, withDestinationIndexPath destinationIndexPath: IndexPath?) -> UITableViewDropProposal {
        
        var dropProposal = UITableViewDropProposal(operation: .cancel)
        guard session.items.count == 1 else { return dropProposal }
        
        if tableView.hasActiveDrag {
            dropProposal = UITableViewDropProposal(operation: .move, intent: .insertAtDestinationIndexPath)
        }
        return dropProposal
    }
        
    func tableView(_ tableView: UITableView, performDropWith coordinator: UITableViewDropCoordinator) {
        guard let sourceIndexPath = coordinator.items.first?.sourceIndexPath else { return }
        let destinationIndexPath: IndexPath
        if let indexPath = coordinator.destinationIndexPath {
            destinationIndexPath = indexPath
        } else {
            let section = tableView.numberOfSections - 1
            let row = tableView.numberOfRows(inSection: section)
            destinationIndexPath = IndexPath(row: row, section: section)
        }
        
        if coordinator.proposal.operation == .move {
            guard let item = coordinator.items.first else { return }
            _ = coordinator.drop(item.dragItem, toRowAt: destinationIndexPath)
            tableView.moveRow(at: sourceIndexPath, to: destinationIndexPath)
        }
    }
    
    func tableView(_ tableView: UITableView, dragPreviewParametersForRowAt indexPath: IndexPath) -> UIDragPreviewParameters? {
        guard let cell = tableView.cellForRow(at: indexPath) as? PlaylistCell else { return nil }
        
        let preview = UIDragPreviewParameters()
        preview.visiblePath = UIBezierPath(roundedRect: cell.contentView.frame, cornerRadius: 12.0)
        preview.backgroundColor = slightlyLighterColour(color: UIColor.braveBackground)
        return preview
    }

    func tableView(_ tableView: UITableView, dropPreviewParametersForRowAt indexPath: IndexPath) -> UIDragPreviewParameters? {
        guard let cell = tableView.cellForRow(at: indexPath) as? PlaylistCell else { return nil }
        
        let preview = UIDragPreviewParameters()
        preview.visiblePath = UIBezierPath(roundedRect: cell.contentView.frame, cornerRadius: 12.0)
        preview.backgroundColor = slightlyLighterColour(color: UIColor.braveBackground)
        return preview
    }
    
    func tableView(_ tableView: UITableView, dragSessionIsRestrictedToDraggingApplication session: UIDragSession) -> Bool {
        true
    }
    
    private func slightlyLighterColour(color: UIColor) -> UIColor {
        let desaturation: CGFloat = 0.5
        var h: CGFloat = 0, s: CGFloat = 0
        var b: CGFloat = 0, a: CGFloat = 0

        guard color.getHue(&h, saturation: &s, brightness: &b, alpha: &a) else {return color}

        return UIColor(hue: h,
                       saturation: max(s - desaturation, 0.0),
                       brightness: b,
                       alpha: a)
    }
}
