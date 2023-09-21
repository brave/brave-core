// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveStrings
import Data
import UIKit
import Favicon

extension TabTrayController: UITableViewDataSource, UITableViewDelegate {

  func numberOfSections(in tableView: UITableView) -> Int {
    sessionList.count
  }
  
  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    if self.hiddenSections.contains(section) {
      return 0
    }
    
    return sessionList[safe: section]?.tabs.count ?? 0
  }
  
  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueReusableCell(for: indexPath) as TabSyncTableViewCell

    configureCell(cell, atIndexPath: indexPath)

    return cell
  }
  
  private func configureCell(_ cell: UITableViewCell, atIndexPath indexPath: IndexPath) {
    guard let cell = cell as? TabSyncTableViewCell else { return }

    guard let distantTab = sessionList[safe: indexPath.section]?.tabs[safe: indexPath.row] else {
      return
    }

    cell.do {
      $0.detailTextLabel?.font = .preferredFont(forTextStyle: .subheadline)
      $0.setLines(distantTab.title, detailText: distantTab.url.absoluteString)
      $0.selectionStyle = .none
    }
    
    cell.imageIconView.do {
      $0.contentMode = .scaleAspectFit
      $0.layer.borderColor = FaviconUX.faviconBorderColor.cgColor
      $0.layer.borderWidth = FaviconUX.faviconBorderWidth
      $0.layer.cornerRadius = 6
      $0.layer.cornerCurve = .continuous
      $0.layer.masksToBounds = true
      $0.loadFavicon(for: distantTab.url, isPrivateBrowsing: tabManager.privateBrowsingManager.isPrivateBrowsing)
    }
  }
  
  func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
    UITableView.automaticDimension
  }

  func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
    UITableView.automaticDimension
  }
  
  func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    guard let sectionDetails = sessionList[safe: section] else {
      return nil
    }
    
    var deviceTypeImage: UIImage?
    
    switch sectionDetails.deviceFormFactor {
    case .phone, .tablet:
      deviceTypeImage = UIImage(braveSystemNamed: "leo.smartphone.tablet-portrait")
    case .desktop:
      deviceTypeImage = UIImage(braveSystemNamed: "leo.laptop")
    default:
      deviceTypeImage = UIImage(braveSystemNamed: "leo.smartphone.laptop")
    }
        
    let headerView = tableView.dequeueReusableHeaderFooter() as TabSyncHeaderView
    let browserColors: some BrowserColors = .standard // Sync devices dont appear in private mode
    
    headerView.do {
      $0.imageIconView.image = deviceTypeImage?.template
      $0.titleLabel.text = sectionDetails.name
      if let modifiedTime = sectionDetails.modifiedTime {
        $0.descriptionLabel.text = modifiedTime.formattedSyncSessionPeriodDate
      }
      $0.isCollapsed = hiddenSections.contains(section)
      $0.section = section
      $0.delegate = self
      $0.backgroundColor = browserColors.chromeBackground
      $0.tintColor = browserColors.chromeBackground
    }
         
    return headerView
  }
  
  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    guard let distantTab = sessionList[safe: indexPath.section]?.tabs[safe: indexPath.row] else {
      return
    }
    
    tabTraySearchController.isActive = false

    if let url = URL(string: distantTab.url.absoluteString) {
      dismiss(animated: true) {
        self.toolbarUrlActionsDelegate?.openInNewTab(url, isPrivate: false)
      }
    }

    tableView.deselectRow(at: indexPath, animated: true)
  }
  
  func tableView(_ tableView: UITableView, contextMenuConfigurationForRowAt indexPath: IndexPath, point: CGPoint) -> UIContextMenuConfiguration? {
    guard let distantTab = sessionList[safe: indexPath.section]?.tabs[safe: indexPath.row] else {
      return nil
    }

    return UIContextMenuConfiguration(identifier: indexPath as NSCopying, previewProvider: nil) { [unowned self] _ in
      let openInNewTabAction = UIAction(
        title: Strings.openNewTabButtonTitle,
        image: UIImage(systemName: "plus.square.on.square"),
        handler: UIAction.deferredActionHandler { _ in
          self.toolbarUrlActionsDelegate?.openInNewTab(distantTab.url, isPrivate: false)
          self.presentingViewController?.dismiss(animated: true)
        })

      let copyAction = UIAction(
        title: Strings.copyLinkActionTitle,
        image: UIImage(systemName: "doc.on.doc"),
        handler: UIAction.deferredActionHandler { _ in
          self.toolbarUrlActionsDelegate?.copy(distantTab.url)
        })

      let shareAction = UIAction(
        title: Strings.shareLinkActionTitle,
        image: UIImage(systemName: "square.and.arrow.up"),
        handler: UIAction.deferredActionHandler { _ in
          self.toolbarUrlActionsDelegate?.share(distantTab.url)
        })

      let urlMenu = UIMenu(title: "", options: .displayInline, children: [openInNewTabAction])
      let linkMenu = UIMenu(title: "", options: .displayInline, children: [copyAction, shareAction])

      return UIMenu(title: distantTab.url.absoluteString, identifier: nil, children: [urlMenu, linkMenu])
    }
  }
}

extension TabTrayController: TabSyncHeaderViewDelegate {
  func toggleSection(_ header: TabSyncHeaderView, section: Int) {
    func indexPathsForSection() -> [IndexPath] {
      var indexPaths = [IndexPath]()

      if let itemCount = sessionList[safe: section]?.tabs.count {
        for row in 0..<itemCount {
          indexPaths.append(IndexPath(row: row, section: section))
        }
      }

      return indexPaths
    }

    if hiddenSections.contains(section) {
      hiddenSections.remove(section)
      tabSyncView.tableView.insertRows(at: indexPathsForSection(), with: .fade)
    } else {
      hiddenSections.insert(section)
      tabSyncView.tableView.deleteRows(at: indexPathsForSection(), with: .fade)
    }
  }
  
  func hideForNow(_ header: TabSyncHeaderView, section: Int) {
    guard let sectionDetails = sessionList[safe: section] else {
      return
    }
    
    braveCore.openTabsAPI.deleteSyncedSession(sectionDetails.sessionTag)
  }
  
  func openAll(_ header: TabSyncHeaderView, section: Int) {
    guard let tabList = sessionList[safe: section]?.tabs else {
      return
    }
    
    let urls: [URL] = tabList.compactMap { $0.url }
    
    dismiss(animated: true) {
      self.toolbarUrlActionsDelegate?.batchOpen(urls)
    }
  }
  
}

extension Date {
  enum TimePeriodOffset {
    case today, yesterday, lastWeek, lastMonth

    var period: Int {
      switch self {
        case .today: return 0
        case .yesterday: return -1
        case .lastWeek: return -7
        case .lastMonth: return -31
      }
    }
  }
  
  var formattedSyncSessionPeriodDate: String {
    let hourFormatter = DateFormatter().then {
      $0.locale = .current
      $0.dateFormat = "h:mm a"
    }
    
    let hourDayFormatter = DateFormatter().then {
      $0.locale = .current
      $0.dateFormat = "EEEE h:mm a"
    }
    
    let fullDateFormatter = DateFormatter().then {
      $0.locale = .current
      $0.dateFormat = "h:mm a MM-dd-yyyy"
    }
        
    if compare(getCurrentDateWith(dayOffset: TimePeriodOffset.today.period)) ==
        ComparisonResult.orderedDescending {
      return String(format: Strings.OpenTabs.openTabsItemLastSyncedTodayTitle, hourFormatter.string(from: self))
    } else if compare(getCurrentDateWith(dayOffset: TimePeriodOffset.yesterday.period)) ==
                ComparisonResult.orderedDescending {
      return String(format: Strings.OpenTabs.openTabsItemLastSyncedYesterdayTitle, hourFormatter.string(from: self))
    } else if compare(getCurrentDateWith(dayOffset: TimePeriodOffset.lastWeek.period)) ==
                ComparisonResult.orderedDescending {
      return String(format: Strings.OpenTabs.openTabsItemLastSyncedLastWeekTitle, hourDayFormatter.string(from: self))
    }
    
    return fullDateFormatter.string(from: self)
  }
  
  var formattedActivePeriodDate: String {
    if compare(getCurrentDateWith(dayOffset: TimePeriodOffset.today.period)) ==
        ComparisonResult.orderedDescending {
      return Strings.OpenTabs.activePeriodDeviceTodayTitle
    } else if compare(getCurrentDateWith(dayOffset: TimePeriodOffset.yesterday.period)) ==
                ComparisonResult.orderedDescending {
      return Strings.OpenTabs.activePeriodDeviceYesterdayTitle
    } else if compare(getCurrentDateWith(dayOffset: TimePeriodOffset.lastWeek.period)) ==
                ComparisonResult.orderedDescending {
      return Strings.OpenTabs.activePeriodDeviceThisWeekTitle
    } else if compare(getCurrentDateWith(dayOffset: TimePeriodOffset.lastMonth.period)) ==
                ComparisonResult.orderedDescending {
      return Strings.OpenTabs.activePeriodDeviceThisMonthTitle
    }
      
    let dateComponents = Calendar(identifier: .gregorian).dateComponents([.day], from: self, to: Date())
      
    return String(format: Strings.OpenTabs.activePeriodDeviceDaysAgoTitle, (dateComponents.day ?? 0))
  }
  
  private func getCurrentDateWith(dayOffset: Int) -> Date {
    let calendar = Calendar(identifier: Calendar.Identifier.gregorian)
    let nowComponents = calendar.dateComponents(
      [Calendar.Component.year, Calendar.Component.month, Calendar.Component.day], from: Date())

    guard let today = calendar.date(from: nowComponents) else {
      return Date()
    }

    return (calendar as NSCalendar).date(
      byAdding: NSCalendar.Unit.day, value: dayOffset, to: today, options: []) ?? Date()
  }

}
