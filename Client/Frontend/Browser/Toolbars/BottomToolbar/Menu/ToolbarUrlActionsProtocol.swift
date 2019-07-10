// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared

protocol ToolbarUrlActionsProtocol where Self: UIViewController {
    var toolbarUrlActionsDelegate: ToolbarUrlActionsDelegate? { get }
    
    func presentLongPressActions(_ gesture: UILongPressGestureRecognizer, urlString: String?,
                                 isPrivateBrowsing: Bool, customActions: [UIAlertAction]?)
}

extension ToolbarUrlActionsProtocol {
    func presentLongPressActions(_ gesture: UILongPressGestureRecognizer, urlString: String?,
                                 isPrivateBrowsing: Bool, customActions: [UIAlertAction]? = nil) {
        
        let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
        alert.title = urlString?.replacingOccurrences(of: "mailto:", with: "").ellipsize(maxLength: 120)
        
        let alertActions = customActions ?? actions(forUrl: URL(string: urlString ?? ""),
                                                    currentTabIsPrivate: isPrivateBrowsing)
        alertActions.forEach { alert.addAction($0) }
        
        if alertActions.isEmpty {
            assertionFailure("Alert actions should not be empty")
            return
        }
        
        let cancelAction = UIAlertAction(title: Strings.CancelButtonTitle, style: .cancel, handler: nil)
        alert.addAction(cancelAction)
        
        // If we're showing an arrow popup, set the anchor to the long press location.
        if let popoverPresentationController = alert.popoverPresentationController {
            popoverPresentationController.sourceView = view
            popoverPresentationController.sourceRect = CGRect(origin: gesture.location(in: view), size: CGSize(width: 0, height: 16))
            popoverPresentationController.permittedArrowDirections = .any
        }
        
        present(alert, animated: true)
    }
    
    private func actions(forUrl url: URL?, currentTabIsPrivate: Bool) -> [UIAlertAction] {
        guard let url = url, let delegate = toolbarUrlActionsDelegate else { return [] }
        
        typealias Action = (title: String, action: () -> Void)
        
        let newTabAction = Action(title: Strings.OpenNewTabButtonTitle,
                                  action: { delegate.openInNewTab(url, isPrivate: currentTabIsPrivate) })
        
        let newPrivateTabAction = currentTabIsPrivate ? nil :
            Action(title: Strings.OpenNewPrivateTabButtonTitle, action: {
                delegate.openInNewTab(url, isPrivate: true)
            })
        
        let copyAction = Action(title: Strings.CopyLinkActionTitle,
                                action: { delegate.copy(url) })
        
        let shareAction = Action(title: Strings.ShareLinkActionTitle,
                                 action: { delegate.share(url) })
        
        return [newTabAction, newPrivateTabAction, copyAction, shareAction].compactMap { $0 }.map { action in
            UIAlertAction(title: action.title, style: .default) { _ in
                self.dismiss(animated: true, completion: action.action)
            }
        }
    }
    
}
