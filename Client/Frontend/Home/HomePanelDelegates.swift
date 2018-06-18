/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Shared
import SnapKit
import UIKit
import Storage

private struct HomePanelViewControllerUX {
    // Height of the top panel switcher button toolbar.
    static let ButtonContainerHeight: CGFloat = 40
    static let ButtonContainerBorderColor = UIColor.Photon.Grey30
    static let BackgroundColorPrivateMode = UIConstants.PrivateModeAssistantToolbarBackgroundColor
    static let ButtonHighlightLineHeight: CGFloat = 2
    static let ButtonSelectionAnimationDuration = 0.2
}

protocol HomePanel: class {
    weak var homePanelDelegate: HomePanelDelegate? { get set }
}

struct HomePanelUX {
    static let EmptyTabContentOffset = -180
}

protocol HomePanelDelegate: class {
    // TODO: Remove sign in/create account delegate methods
    func homePanelDidRequestToSignIn(_ homePanel: HomePanel)
    func homePanelDidRequestToCreateAccount(_ homePanel: HomePanel)
    func homePanelDidRequestToOpenInNewTab(_ url: URL, isPrivate: Bool)
    func homePanelDidRequestToCopyURL(_ url: URL)
    func homePanelDidRequestToShareURL(_ url: URL)
    func homePanelDidRequestToBatchOpenURLs(_ urls: [URL])
    func homePanel(_ homePanel: HomePanel, didSelectURL url: URL, visitType: VisitType)
    func homePanel(_ homePanel: HomePanel, didSelectURLString url: String, visitType: VisitType)
}

struct HomePanelState {
    var selectedIndex: Int = 0
}

enum HomePanelType: Int {
    case topSites = 0
    case bookmarks = 1
    case history = 2
    case readingList = 3
    case downloads = 4

    var localhostURL: URL {
        return URL(string: "#panel=\(self.rawValue)", relativeTo: UIConstants.AboutHomePage as URL)!
    }
}

protocol HomePanelContextMenu {
    func getSiteDetails(for indexPath: IndexPath) -> Site?
    func getContextMenuActions(for site: Site, with indexPath: IndexPath) -> [PhotonActionSheetItem]?
    func presentContextMenu(for indexPath: IndexPath)
    func presentContextMenu(for site: Site, with indexPath: IndexPath, completionHandler: @escaping () -> PhotonActionSheet?)
}

extension HomePanelContextMenu {
    func presentContextMenu(for indexPath: IndexPath) {
        guard let site = getSiteDetails(for: indexPath) else { return }

        presentContextMenu(for: site, with: indexPath, completionHandler: {
            return self.contextMenu(for: site, with: indexPath)
        })
    }

    func contextMenu(for site: Site, with indexPath: IndexPath) -> PhotonActionSheet? {
        guard let actions = self.getContextMenuActions(for: site, with: indexPath) else { return nil }

        let contextMenu = PhotonActionSheet(site: site, actions: actions)
        contextMenu.modalPresentationStyle = .overFullScreen
        contextMenu.modalTransitionStyle = .crossDissolve

        return contextMenu
    }

    func getDefaultContextMenuActions(for site: Site, homePanelDelegate: HomePanelDelegate?) -> [PhotonActionSheetItem]? {
        guard let siteURL = URL(string: site.url) else { return nil }

        let openInNewTabAction = PhotonActionSheetItem(title: Strings.OpenInNewTabContextMenuTitle, iconString: "quick_action_new_tab") { action in
            homePanelDelegate?.homePanelDidRequestToOpenInNewTab(siteURL, isPrivate: false)
        }

        let openInNewPrivateTabAction = PhotonActionSheetItem(title: Strings.OpenInNewPrivateTabContextMenuTitle, iconString: "quick_action_new_private_tab") { action in
            homePanelDelegate?.homePanelDidRequestToOpenInNewTab(siteURL, isPrivate: true)
        }

        return [openInNewTabAction, openInNewPrivateTabAction]
    }
}
