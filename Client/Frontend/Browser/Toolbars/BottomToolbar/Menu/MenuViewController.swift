// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Storage
import Shared
import BraveUI

private class MenuCell: UITableViewCell {
    let iconView = UIImageView()
    let labelView = UILabel()
    let iconLength: CGFloat = 50.0
    
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        contentView.addSubview(iconView)
        contentView.addSubview(labelView)
        
        iconView.contentMode = .center
        iconView.snp.makeConstraints {
            $0.leading.top.bottom.equalTo(self)
            $0.width.equalTo(iconLength)
        }
        labelView.snp.makeConstraints {
            $0.centerY.equalTo(self)
            $0.trailing.equalTo(self).inset(12)
            $0.leading.equalTo(iconView.snp.trailing)
        }
        separatorInset = UIEdgeInsets(top: 0, left: iconLength, bottom: 0, right: 0)
    }
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    @available(*, unavailable)
    override var textLabel: UILabel? {
        return nil
    }
    @available(*, unavailable)
    override var imageView: UIImageView? {
        return nil
    }
}

class MenuViewController: UITableViewController {
    
    private struct UX {
        static let rowHeight: CGFloat = 45
        static let separatorColor = UIColor(white: 0.0, alpha: 0.1)
        static let topBottomInset: CGFloat = 5
    }
    
    private enum MenuButtons: Int, CaseIterable {
        case settings, history, bookmarks, downloads, add, share
        
        var title: String {
            switch self {
            case .bookmarks: return Strings.bookmarksMenuItem
            case .history: return Strings.historyMenuItem
            case .settings: return Strings.settingsMenuItem
            case .add: return Strings.addToMenuItem
            case .share: return Strings.shareWithMenuItem
            case .downloads: return Strings.downloadsMenuItem
            }
        }
        
        var icon: UIImage {
            switch self {
            case .bookmarks: return #imageLiteral(resourceName: "menu_bookmarks").template
            case .history: return #imageLiteral(resourceName: "menu-history").template
            case .settings: return #imageLiteral(resourceName: "menu-settings").template
            case .add: return #imageLiteral(resourceName: "menu-add-bookmark").template
            case .share: return #imageLiteral(resourceName: "nav-share").template
            case .downloads: return #imageLiteral(resourceName: "menu-downloads").template
            }
        }
    }
    
    private let bvc: BrowserViewController
    private let tab: Tab?
    
    private lazy var visibleButtons: [MenuButtons] = {
        let allButtons = MenuButtons.allCases
        
        // Don't show url buttons if there is no url to pick(like on home screen)
        var allWithoutUrlButtons = allButtons
        allWithoutUrlButtons.removeAll { $0 == .add || $0 == .share }
        
        guard let url = tab?.url, (!url.isLocal || url.isReaderModeURL) else {
            return allWithoutUrlButtons
        }
        return allButtons
    }()
    
    // MARK: - Init
    
    init(bvc: BrowserViewController, tab: Tab?) {
        self.bvc = bvc
        self.tab = tab

        super.init(nibName: nil, bundle: nil)
    }
    
    @available(*, unavailable)
    required init?(coder aDecoder: NSCoder) {
        fatalError()
    }
    
    // MARK: - Lifecycle
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        tableView.separatorColor = UX.separatorColor
        tableView.rowHeight = UX.rowHeight
        tableView.backgroundColor = .clear
        
        tableView.contentInset = UIEdgeInsets(top: UX.topBottomInset, left: 0,
                                              bottom: UX.topBottomInset, right: 0)
        
        tableView.showsVerticalScrollIndicator = false
        tableView.isScrollEnabled = false
        
        // Hide separator line of the last cell.
        tableView.tableFooterView =
            UIView(frame: CGRect(x: 0, y: 0, width: tableView.frame.size.width, height: 1))
        
        let size = CGSize(width: 200, height: UIScreen.main.bounds.height)
        
        let fit = view.systemLayoutSizeFitting(
            size,
            withHorizontalFittingPriority: .required,
            verticalFittingPriority: .defaultHigh
        )
        
        preferredContentSize = CGSize(width: fit.width, height: fit.height + UX.topBottomInset * 2)
        
    }
    
    // MARK: - Table view data source
    
    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        
        guard let cell = tableView.cellForRow(at: indexPath) else { return }
        
        guard let button = MenuButtons(rawValue: cell.tag) else {
            assertionFailure("No cell with \(cell.tag) tag.")
            return
        }
        
        switch button {
        case .bookmarks: openBookmarks()
        case .history: openHistory()
        case .settings: openSettings()
        case .add: openAddBookmark()
        case .share: openShareSheet()
        case .downloads: openDownloads()
        }
    }
    
    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return visibleButtons.count
    }
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let button = visibleButtons[indexPath.row]
        let cell = MenuCell()
        
        cell.labelView.text = button.title
        cell.iconView.image = button.icon
        
        let homeColor = Theme.of(tab).colors.tints.home
        cell.iconView.tintColor = homeColor.withAlphaComponent(0.6)
        cell.labelView.textColor = homeColor
        cell.tag = button.rawValue
        cell.backgroundColor = .clear
        
        return cell
    }
    
    override func tableView(_ tableView: UITableView, didHighlightRowAt indexPath: IndexPath) {
        let cell = tableView.cellForRow(at: indexPath)
        cell?.contentView.backgroundColor = Theme.of(tab).colors.home.withAlphaComponent(0.5)
    }
    
    override func tableView(_ tableView: UITableView, didUnhighlightRowAt indexPath: IndexPath) {
        let cell = tableView.cellForRow(at: indexPath)
        cell?.contentView.backgroundColor = .clear
    }
    
    // MARK: - Actions
    
    private enum DoneButtonPosition { case left, right }
    private typealias DoneButton = (style: UIBarButtonItem.SystemItem, position: DoneButtonPosition)
    
    private func open(_ viewController: UIViewController, doneButton: DoneButton,
                      allowSwipeToDismiss: Bool = true) {
        let nav = SettingsNavigationController(rootViewController: viewController)
        if UIDevice.current.userInterfaceIdiom == .phone {
            nav.modalPresentationStyle = .fullScreen
        } else {
            nav.modalPresentationStyle = .formSheet
        }
        
        if #available(iOS 13.0, *) {
            nav.isModalInPresentation = !allowSwipeToDismiss
        }
        
        let button = UIBarButtonItem(barButtonSystemItem: doneButton.style, target: nav, action: #selector(nav.done))
        
        switch doneButton.position {
        case .left: nav.navigationBar.topItem?.leftBarButtonItem = button
        case .right: nav.navigationBar.topItem?.rightBarButtonItem = button
        }
        
        dismissView()
        bvc.present(nav, animated: true)
    }
    
    private func openBookmarks() {
        let vc = BookmarksViewController(mode: .bookmarks(inFolder: nil), isPrivateBrowsing: PrivateBrowsingManager.shared.isPrivateBrowsing)
        vc.toolbarUrlActionsDelegate = bvc
        
        open(vc, doneButton: DoneButton(style: .done, position: .right))
    }
    
    private func openDownloads() {
        let vc = DownloadsPanel(profile: bvc.profile)
        let currentTheme = Theme.of(bvc.tabManager.selectedTab)
        vc.applyTheme(currentTheme)
        
        open(vc, doneButton: DoneButton(style: .done, position: .right))
    }
    
    private func openAddBookmark() {
        guard let title = tab?.displayTitle, let url = tab?.url else { return }
        
        let bookmarkUrl = url.decodeReaderModeURL ?? url
        
        let mode = BookmarkEditMode.addBookmark(title: title, url: bookmarkUrl.absoluteString)
        
        let vc = AddEditBookmarkTableViewController(mode: mode)
        
        open(vc, doneButton: DoneButton(style: .cancel, position: .left))

    }
    
    private func openHistory() {
        let vc = HistoryViewController(isPrivateBrowsing: PrivateBrowsingManager.shared.isPrivateBrowsing)
        vc.toolbarUrlActionsDelegate = bvc
        
        open(vc, doneButton: DoneButton(style: .done, position: .right))
    }
    
    private func openSettings() {
        let vc = SettingsViewController(profile: bvc.profile, tabManager: bvc.tabManager, rewards: bvc.rewards)
        vc.settingsDelegate = bvc
        open(vc, doneButton: DoneButton(style: .done, position: .right),
             allowSwipeToDismiss: false)
    }
    
    private func openShareSheet() {
        dismissView()
        bvc.tabToolbarDidPressShare()
    }
    
    @objc func dismissView() {
        dismiss(animated: true)
    }
}

// MARK: - PopoverContentComponent

extension MenuViewController: PopoverContentComponent {
    var extendEdgeIntoArrow: Bool { return false }
}
