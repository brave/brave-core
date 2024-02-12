/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import BraveShared
import Preferences
import Storage
import Data
import CoreData
import BraveCore
import Favicon
import UIKit
import DesignSystem
import ScreenTime

class HistoryViewController: SiteTableViewController, ToolbarUrlActionsProtocol {

  weak var toolbarUrlActionsDelegate: ToolbarUrlActionsDelegate?

  private lazy var emptyStateOverlayView = EmptyStateOverlayView(
    overlayDetails: EmptyOverlayStateDetails(
      title: Preferences.Privacy.privateBrowsingOnly.value
        ? Strings.History.historyPrivateModeOnlyStateTitle
        : Strings.History.historyEmptyStateTitle,
      icon: UIImage(named: "emptyHistory", in: .module, compatibleWith: nil)))
  
  private let historyAPI: BraveHistoryAPI
  private let tabManager: TabManager
  private var historyFRC: HistoryV2FetchResultsController?

  private let isPrivateBrowsing: Bool  /// Certain bookmark actions are different in private browsing mode.
  private let isModallyPresented: Bool
  private var isHistoryRefreshing = false

  private var searchHistoryTimer: Timer?
  private var isHistoryBeingSearched = false
  private let searchController = UISearchController(searchResultsController: nil)
  private var searchQuery = ""

  init(isPrivateBrowsing: Bool, isModallyPresented: Bool = false, historyAPI: BraveHistoryAPI, tabManager: TabManager) {
    self.isPrivateBrowsing = isPrivateBrowsing
    self.isModallyPresented = isModallyPresented
    self.historyAPI = historyAPI
    self.tabManager = tabManager
    super.init(nibName: nil, bundle: nil)

    historyFRC = historyAPI.frc()
    historyFRC?.delegate = self
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    applyTheme()

    tableView.do {
      $0.accessibilityIdentifier = "History List"
      $0.sectionHeaderTopPadding = 5
    }

    navigationItem.do {
      if !Preferences.Privacy.privateBrowsingOnly.value {
        $0.searchController = searchController
        $0.hidesSearchBarWhenScrolling = false
        $0.rightBarButtonItem =
          UIBarButtonItem(image: UIImage(braveSystemNamed: "leo.trash")!.template, style: .done, target: self, action: #selector(performDeleteAll))
      }
      
      if isModallyPresented {
        navigationItem.leftBarButtonItem = UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(performDone))
      }
    }

    definesPresentationContext = true
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)

    refreshHistory()
  }

  private func applyTheme() {
    title = Strings.historyScreenTitle

    searchController.do {
      $0.searchBar.autocapitalizationType = .none
      $0.searchResultsUpdater = self
      $0.obscuresBackgroundDuringPresentation = false
      $0.searchBar.placeholder = Strings.History.historySearchBarTitle
      $0.delegate = self
      $0.hidesNavigationBarDuringPresentation = true
    }
  }

  private func refreshHistory() {
    if isHistoryBeingSearched {
      return
    }

    if Preferences.Privacy.privateBrowsingOnly.value {
      showEmptyPanelState()
    } else {
      if !isHistoryRefreshing {
        isLoading = true
        isHistoryRefreshing = true

        historyAPI.waitForHistoryServiceLoaded { [weak self] in
          guard let self = self else { return }

          self.reloadData() {
            self.isHistoryRefreshing = false
            self.isLoading = false
          }
        }
      }
    }
  }

  private func reloadData(with query: String = "", _ completion: @escaping () -> Void) {
    // Recreate the frc if it was previously removed
    if historyFRC == nil {
      historyFRC = historyAPI.frc()
      historyFRC?.delegate = self
    }

    historyFRC?.performFetch(withQuery: query) { [weak self] in
      guard let self = self else { return }

      self.tableView.reloadData()
      self.updateEmptyPanelState()

      completion()
    }
  }

  private func reloadDataAndShowLoading(with query: String) {
    isLoading = true
    reloadData(with: query) { [weak self] in
      self?.isLoading = false
    }
  }

  private func updateEmptyPanelState() {
    if historyFRC?.fetchedObjectsCount == 0 {
      showEmptyPanelState()
    } else {
      emptyStateOverlayView.removeFromSuperview()
    }
  }

  private func showEmptyPanelState() {
    if emptyStateOverlayView.superview == nil {

      if isHistoryBeingSearched {
        emptyStateOverlayView.updateInfoLabel(with: Strings.noSearchResultsfound)
      } else {
        emptyStateOverlayView.updateInfoLabel(
          with: Preferences.Privacy.privateBrowsingOnly.value
            ? Strings.History.historyPrivateModeOnlyStateTitle
            : Strings.History.historyEmptyStateTitle)
      }

      view.addSubview(emptyStateOverlayView)
      view.bringSubviewToFront(emptyStateOverlayView)
      emptyStateOverlayView.snp.makeConstraints { make -> Void in
        make.edges.equalTo(tableView)
      }
    }
  }

  private func invalidateSearchTimer() {
    if searchHistoryTimer != nil {
      searchHistoryTimer?.invalidate()
      searchHistoryTimer = nil
    }
  }
  
  // MARK: Actions

  @objc private func performDeleteAll() {
    let style: UIAlertController.Style = UIDevice.current.userInterfaceIdiom == .pad ? .alert : .actionSheet
    let alert = UIAlertController(
      title: Strings.History.historyClearAlertTitle, message: Strings.History.historyClearAlertDescription, preferredStyle: style)

    alert.addAction(
      UIAlertAction(
        title: Strings.History.historyClearActionTitle, style: .destructive,
        handler: { [weak self] _ in
          guard let self = self, let allHistoryItems = historyFRC?.fetchedObjects  else {
            return
          }

          // Deleting Local History
          self.historyAPI.deleteAll {
            // Clearing Tab History with entire history entry
            self.tabManager.clearTabHistory() {
              self.refreshHistory()
            }
            
            // Clearing History should clear Recently Closed
            RecentlyClosed.removeAll()
            
            // Donate Clear Browser History for suggestions
            let clearBrowserHistoryActivity = ActivityShortcutManager.shared.createShortcutActivity(type: .clearBrowsingHistory)
            self.userActivity = clearBrowserHistoryActivity
            clearBrowserHistoryActivity.becomeCurrent()
          }
          
          // Asking Sync Engine To Remove Visits
          for historyItems in allHistoryItems {
            self.historyAPI.removeHistory(historyItems)
          }
        }))
    alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))

    present(alert, animated: true, completion: nil)
  }
  
  @objc private func performDone() {
    dismiss(animated: true)
  }
  
  // MARK: UITableViewDelegate - UITableViewDataSource

  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = super.tableView(tableView, cellForRowAt: indexPath)
    configureCell(cell, atIndexPath: indexPath)

    return cell
  }

  func configureCell(_ cell: UITableViewCell, atIndexPath indexPath: IndexPath) {
    // Make sure History at index path exists,
    // `frc.object(at:)` crashes otherwise, doesn't fail safely with nil
    if let objectsCount = historyFRC?.fetchedObjectsCount, indexPath.row >= objectsCount {
      assertionFailure("History FRC index out of bounds")
      return
    }

    guard let cell = cell as? TwoLineTableViewCell else { return }

    if !tableView.isEditing {
      cell.gestureRecognizers?.forEach { cell.removeGestureRecognizer($0) }
    }

    guard let historyItem = historyFRC?.object(at: indexPath) else { return }

    cell.do {
      $0.backgroundColor = UIColor.clear
      $0.setLines(historyItem.title, detailText: historyItem.url.absoluteString)

      $0.imageView?.contentMode = .scaleAspectFit
      $0.imageView?.layer.borderColor = FaviconUX.faviconBorderColor.cgColor
      $0.imageView?.layer.borderWidth = FaviconUX.faviconBorderWidth
      $0.imageView?.layer.cornerRadius = 6
      $0.imageView?.layer.cornerCurve = .continuous
      $0.imageView?.layer.masksToBounds = true

      let domain = Domain.getOrCreate(
        forUrl: historyItem.url,
        persistent: !isPrivateBrowsing)

      if domain.url?.asURL != nil {
        cell.imageView?.loadFavicon(for: historyItem.url, isPrivateBrowsing: isPrivateBrowsing)
      } else {
        cell.imageView?.clearMonogramFavicon()
        cell.imageView?.image = Favicon.defaultImage
      }
    }
  }

  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    guard let historyItem = historyFRC?.object(at: indexPath) else { return }

    if isHistoryBeingSearched {
      searchController.isActive = false
    }

    if let url = URL(string: historyItem.url.absoluteString) {
      // Donate Custom Intent Open Website
      if url.isSecureWebPage(), !isPrivateBrowsing {
        ActivityShortcutManager.shared.donateCustomIntent(for: .openHistory, with: url.absoluteString)
      }
      
      dismiss(animated: true) {
        self.toolbarUrlActionsDelegate?.select(url: url, isUserDefinedURLNavigation: false)
      }
    }

    tableView.deselectRow(at: indexPath, animated: true)
  }

  func numberOfSections(in tableView: UITableView) -> Int {
    return historyFRC?.sectionCount ?? 0
  }

  func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
    return historyFRC?.titleHeader(for: section)
  }

  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return historyFRC?.objectCount(for: section) ?? 0
  }

  func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
    switch editingStyle {
    case .delete:
      guard let historyItem = historyFRC?.object(at: indexPath) else { return }
      historyAPI.removeHistory(historyItem)
      
      // Reoving a history item should remove its corresponded Recently Closed item
      RecentlyClosed.remove(with: historyItem.url.absoluteString)
      
      do {
        let screenTimeHistory = try STWebHistory(bundleIdentifier: Bundle.main.bundleIdentifier!)
        screenTimeHistory.deleteHistory(for: historyItem.url)
      } catch {
        assertionFailure("STWebHistory could not be initialized: \(error)")
      }
      
      if isHistoryBeingSearched {
        reloadDataAndShowLoading(with: searchQuery)
      } else {
        refreshHistory()
      }
    default:
      break
    }
  }

  func tableView(_ tableView: UITableView, contextMenuConfigurationForRowAt indexPath: IndexPath, point: CGPoint) -> UIContextMenuConfiguration? {
    guard let historyItemURL = historyFRC?.object(at: indexPath)?.url else {
      return nil
    }

    return UIContextMenuConfiguration(identifier: indexPath as NSCopying, previewProvider: nil) { [unowned self] _ in
      let openInNewTabAction = UIAction(
        title: Strings.openNewTabButtonTitle,
        image: UIImage(systemName: "plus.square.on.square"),
        handler: UIAction.deferredActionHandler { _ in
          self.toolbarUrlActionsDelegate?.openInNewTab(historyItemURL, isPrivate: self.isPrivateBrowsing)
          self.presentingViewController?.dismiss(animated: true)
        })

      let newPrivateTabAction = UIAction(
        title: Strings.openNewPrivateTabButtonTitle,
        image: UIImage(systemName: "plus.square.fill.on.square.fill"),
        handler: UIAction.deferredActionHandler { [unowned self] _ in
          if !isPrivateBrowsing, Preferences.Privacy.privateBrowsingLock.value {
            self.askForLocalAuthentication { [weak self] success, error in
              if success {
                self?.toolbarUrlActionsDelegate?.openInNewTab(historyItemURL, isPrivate: true)
              }
            }
          } else {
            self.toolbarUrlActionsDelegate?.openInNewTab(historyItemURL, isPrivate: true)
          }
        })

      let copyAction = UIAction(
        title: Strings.copyLinkActionTitle,
        image: UIImage(systemName: "doc.on.doc"),
        handler: UIAction.deferredActionHandler { _ in
          self.toolbarUrlActionsDelegate?.copy(historyItemURL)
        })

      let shareAction = UIAction(
        title: Strings.shareLinkActionTitle,
        image: UIImage(systemName: "square.and.arrow.up"),
        handler: UIAction.deferredActionHandler { _ in
          self.toolbarUrlActionsDelegate?.share(historyItemURL)
        })

      var newTabActionMenu: [UIAction] = [openInNewTabAction]

      if !isPrivateBrowsing {
        newTabActionMenu.append(newPrivateTabAction)
      }

      let urlMenu = UIMenu(title: "", options: .displayInline, children: newTabActionMenu)
      let linkMenu = UIMenu(title: "", options: .displayInline, children: [copyAction, shareAction])

      return UIMenu(title: historyItemURL.absoluteString, identifier: nil, children: [urlMenu, linkMenu])
    }
  }
}

// MARK: - HistoryV2FetchResultsDelegate

extension HistoryViewController: HistoryV2FetchResultsDelegate {

  func controllerWillChangeContent(_ controller: HistoryV2FetchResultsController) {
    tableView.beginUpdates()
  }

  func controllerDidChangeContent(_ controller: HistoryV2FetchResultsController) {
    tableView.endUpdates()
  }

  func controller(_ controller: HistoryV2FetchResultsController, didChange anObject: Any, at indexPath: IndexPath?, for type: NSFetchedResultsChangeType, newIndexPath: IndexPath?) {
    switch type {
    case .insert:
      if let indexPath = newIndexPath {
        tableView.insertRows(at: [indexPath], with: .automatic)
      }
    case .delete:
      if let indexPath = indexPath {
        tableView.deleteRows(at: [indexPath], with: .automatic)
      }
    case .update:
      if let indexPath = indexPath, let cell = tableView.cellForRow(at: indexPath) {
        configureCell(cell, atIndexPath: indexPath)
      }
    case .move:
      if let indexPath = indexPath {
        tableView.deleteRows(at: [indexPath], with: .automatic)
      }

      if let newIndexPath = newIndexPath {
        tableView.insertRows(at: [newIndexPath], with: .automatic)
      }
    @unknown default:
      assertionFailure()
    }
    updateEmptyPanelState()
  }

  func controller(_ controller: HistoryV2FetchResultsController, didChange sectionInfo: NSFetchedResultsSectionInfo, atSectionIndex sectionIndex: Int, for type: NSFetchedResultsChangeType) {
    switch type {
    case .insert:
      let sectionIndexSet = IndexSet(integer: sectionIndex)
      self.tableView.insertSections(sectionIndexSet, with: .fade)
    case .delete:
      let sectionIndexSet = IndexSet(integer: sectionIndex)
      self.tableView.deleteSections(sectionIndexSet, with: .fade)
    default: break
    }
  }

  func controllerDidReloadContents(_ controller: HistoryV2FetchResultsController) {
    refreshHistory()
  }
}

// MARK: UISearchResultUpdating

extension HistoryViewController: UISearchResultsUpdating {

  func updateSearchResults(for searchController: UISearchController) {
    guard let query = searchController.searchBar.text else { return }

    invalidateSearchTimer()

    searchHistoryTimer =
      Timer.scheduledTimer(timeInterval: 0.1, target: self, selector: #selector(fetchSearchResults(timer:)), userInfo: query, repeats: false)
  }

  @objc private func fetchSearchResults(timer: Timer) {
    guard let query = timer.userInfo as? String else {
      searchQuery = ""
      return
    }

    searchQuery = query
    reloadDataAndShowLoading(with: searchQuery)
  }
}

// MARK: UISearchControllerDelegate

extension HistoryViewController: UISearchControllerDelegate {

  func willPresentSearchController(_ searchController: UISearchController) {
    isHistoryBeingSearched = true
    searchQuery = ""
    tableView.setEditing(false, animated: true)
    tableView.reloadData()
  }

  func willDismissSearchController(_ searchController: UISearchController) {
    invalidateSearchTimer()

    isHistoryBeingSearched = false
    tableView.reloadData()
  }
}
