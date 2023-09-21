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
    
    private(set) var tableView = UITableView(frame: .zero, style: .insetGrouped)

    var actionHandler: ((SyncStatusState) -> Void)?
    
    private var noSyncTabsOverlayView = EmptyStateOverlayView(
      overlayDetails: EmptyOverlayStateDetails())
    
    override init(frame: CGRect) {
      super.init(frame: frame)
      
      backgroundColor = .clear
      
      addSubview(tableView)
      tableView.snp.makeConstraints { make in
        make.edges.equalTo(self)
      }

      tableView.do {
        $0.register(TabSyncTableViewCell.self)
        $0.registerHeaderFooter(TabSyncHeaderView.self)
        $0.backgroundColor = .clear
        $0.estimatedRowHeight = SiteTableViewControllerUX.rowHeight
        $0.estimatedSectionHeaderHeight = SiteTableViewControllerUX.rowHeight
        $0.separatorColor = .braveSeparator
        $0.cellLayoutMarginsFollowReadableWidth = false
        $0.sectionHeaderTopPadding = UX.sectionTopPadding
      }
      
      let tableTitleLabel = UILabel().then {
        $0.textColor = .braveLabel
        $0.textAlignment = .left
        $0.font = .preferredFont(for: .title2, weight: .bold)
        $0.text = Strings.OpenTabs.openTabsListTableHeaderTitle
      }
                 
      let headerView = UIView(frame: .init(width: tableView.frame.width, height: 30))
      headerView.addSubview(tableTitleLabel)

      tableTitleLabel.snp.makeConstraints {
        $0.leading.equalToSuperview().inset(16)
        $0.top.bottom.trailing.equalToSuperview()
      }
      
      tableView.tableHeaderView = headerView

      // Set an empty footer to prevent empty cells from appearing in the list.
      tableView.tableFooterView = UIView()
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) { fatalError() }
    
    private func createNewEmptyStateView(for state: SyncStatusState) -> EmptyStateOverlayView {
      switch state {
      case .noSyncChain:
        let noSyncChainEmptyStateView = EmptyStateOverlayView(
          overlayDetails: EmptyOverlayStateDetails(
            title: Strings.OpenTabs.noSyncSessionPlaceHolderViewTitle,
            description: Strings.OpenTabs.noSyncChainPlaceHolderViewDescription,
            icon: UIImage(named: "sync-settings", in: .module, compatibleWith: nil),
            buttonText: "\(Strings.OpenTabs.syncChainStartButtonTitle) →",
            action: { [weak self] in
              self?.actionHandler?(.noSyncChain)
            })
          )
        
        return noSyncChainEmptyStateView
      case .openTabsDisabled:
        let disabledOpenTabsEmptyStateView = EmptyStateOverlayView(
          overlayDetails: EmptyOverlayStateDetails(
            title: Strings.OpenTabs.noSyncSessionPlaceHolderViewTitle,
            description: Strings.OpenTabs.enableOpenTabsPlaceHolderViewDescription,
            icon: UIImage(named: "sync-settings", in: .module, compatibleWith: nil),
            buttonText: Strings.OpenTabs.tabSyncEnableButtonTitle,
            action: { [weak self] in
              self?.actionHandler?(.openTabsDisabled)
            },
            actionDescription: Strings.OpenTabs.noSyncSessionPlaceHolderViewAdditionalDescription)
          )
        
        return disabledOpenTabsEmptyStateView
      default:
        let noSessionsEmptyStateView = EmptyStateOverlayView(
          overlayDetails: EmptyOverlayStateDetails(
            title: Strings.OpenTabs.noSyncSessionPlaceHolderViewTitle,
            description: Strings.OpenTabs.noSyncSessionPlaceHolderViewDescription,
            icon: UIImage(named: "sync-settings", in: .module, compatibleWith: nil),
            buttonText: Strings.OpenTabs.showSettingsSyncButtonTitle,
            action: { [weak self] in
              self?.actionHandler?(.noSyncedSessions)
            },
            actionDescription: Strings.OpenTabs.noSyncSessionPlaceHolderViewAdditionalDescription)
          )
        
        return noSessionsEmptyStateView
      }
    }
    
    /// Update visibility of view shown when no synced session exists
    /// This view contains information about  how to join sync chain and enable open tabs
    /// - Parameter isHidden: Boolean to set isHidden
    func updateSyncStatusPanel(for state: SyncStatusState) {
      noSyncTabsOverlayView.removeFromSuperview()
      
      if state != .activeSessions {
        noSyncTabsOverlayView.removeFromSuperview()
        noSyncTabsOverlayView = createNewEmptyStateView(for: state)
        
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
