// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import BraveUI

extension TabTrayController {

  class TabSyncContainerView: UIView {
    
    // MARK: UX

    struct UX {
      static let sectionTopPadding = 5.0
    }

    private(set) var tableView = UITableView()
    
    private(set) lazy var noSyncTabsOverlayView = EmptyStateOverlayView(
      title: Strings.OpenTabs.noSyncSessionPlaceHolderViewTitle,
      description: Strings.OpenTabs.noSyncSessionPlaceHolderViewDescription,
      icon: UIImage(systemName: "laptopcomputer.and.iphone"))

    override init(frame: CGRect) {
      super.init(frame: frame)

      backgroundColor = .braveBackground
      
      addSubview(tableView)
      tableView.snp.makeConstraints { make in
        make.edges.equalTo(self)
      }

      tableView.do {
        $0.register(TabSyncTableViewCell.self)
        $0.registerHeaderFooter(TabSyncHeaderView.self)
        $0.layoutMargins = .zero
        $0.backgroundColor = .secondaryBraveBackground
        $0.estimatedRowHeight = SiteTableViewControllerUX.rowHeight
        $0.estimatedSectionHeaderHeight = SiteTableViewControllerUX.rowHeight
        $0.separatorColor = .braveSeparator
        $0.cellLayoutMarginsFollowReadableWidth = false
        if #available(iOS 15.0, *) {
          $0.sectionHeaderTopPadding = UX.sectionTopPadding
        }
      }

      // Set an empty footer to prevent empty cells from appearing in the list.
      tableView.tableFooterView = UIView()
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) { fatalError() }
    
    /// Update visibility of view shown when no synced session exists
    /// This view contains information about  how to join sync chain and enable open tabs
    /// - Parameter isHidden: Boolean to set isHidden
    func updateNoSyncPanelState(isHidden: Bool) {
      if isHidden {
        noSyncTabsOverlayView.removeFromSuperview()
      } else {
        if noSyncTabsOverlayView.superview == nil {
          addSubview(noSyncTabsOverlayView)
          bringSubviewToFront(noSyncTabsOverlayView)
          
          noSyncTabsOverlayView.snp.makeConstraints {
            $0.edges.equalTo(tableView)
          }
        }
      }
    }
  }
}
