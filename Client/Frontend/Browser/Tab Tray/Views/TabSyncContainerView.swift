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
    
    // MARK: SyncActionType
    
    enum SyncActionType {
      case noSyncChain, openTabsDisabled
    }

    private(set) var tableView = UITableView()
    
    var actionHandler: ((SyncActionType) -> Void)?
    
    private var noSyncTabsOverlayView = EmptyStateOverlayView()
    
    override init(frame: CGRect) {
      super.init(frame: frame)
      
      noSyncTabsOverlayView = createNewPanelStateView()

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
    
    private func createNewPanelStateView() -> EmptyStateOverlayView {
      if Preferences.Chromium.syncEnabled.value, !Preferences.Chromium.syncOpenTabsEnabled.value {
        let disabledOpenTabsEmptyStateView = EmptyStateOverlayView(
          title: Strings.OpenTabs.noSyncSessionPlaceHolderViewTitle,
          description: Strings.OpenTabs.noSyncSessionPlaceHolderViewDescription,
          icon: UIImage(named: "sync-settings", in: .current, compatibleWith: nil),
          buttonText: Strings.OpenTabs.tabSyncEnableButtonTitle,
          action: { [weak self] in
            self?.actionHandler?(.openTabsDisabled)
          },
          actionDescription: Strings.OpenTabs.noSyncSessionPlaceHolderViewAdditionalDescription)
        
        return disabledOpenTabsEmptyStateView
      }
      
      let noSyncChainEmptyStateView = EmptyStateOverlayView(
        title: Strings.OpenTabs.noSyncSessionPlaceHolderViewTitle,
        description: Strings.OpenTabs.noSyncChainPlaceHolderViewDescription,
        icon: UIImage(named: "sync-settings", in: .current, compatibleWith: nil),
        buttonText: "\(Strings.OpenTabs.syncChainStartButtonTitle) →",
        action: { [weak self] in
          self?.actionHandler?(.noSyncChain)
        })
      
      return noSyncChainEmptyStateView
    }
    
    /// Update visibility of view shown when no synced session exists
    /// This view contains information about  how to join sync chain and enable open tabs
    /// - Parameter isHidden: Boolean to set isHidden
    func updateNoSyncPanelState(isHidden: Bool) {
      noSyncTabsOverlayView.removeFromSuperview()
      
      if !isHidden {
        noSyncTabsOverlayView.removeFromSuperview()
        noSyncTabsOverlayView = createNewPanelStateView()
        
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
