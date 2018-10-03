/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import BraveShared
import Storage
import Data
import CoreData

private struct HistoryViewControllerUX {
  static let WelcomeScreenPadding: CGFloat = 15
  static let WelcomeScreenItemTextColor = UIColor.gray
  static let WelcomeScreenItemWidth = 170
}

class HistoryViewController: SiteTableViewController {
  weak var linkNavigationDelegate: LinkNavigationDelegate?
  fileprivate lazy var emptyStateOverlayView: UIView = self.createEmptyStateOverview()
  var frc: NSFetchedResultsController<History>?
  
  let tabState: TabState
  
  init(tabState: TabState) {
    self.tabState = tabState
    
    super.init(nibName: nil, bundle: nil)
    
    NotificationCenter.default.addObserver(self, selector: #selector(HistoryViewController.notificationReceived(_:)), name: .DynamicFontChanged, object: nil)
  }
  
  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  deinit {
    NotificationCenter.default.removeObserver(self, name: .DynamicFontChanged, object: nil)
  }
  
  override func viewDidLoad() {
    frc = History.frc()
    frc!.delegate = self
    super.viewDidLoad()
    self.tableView.accessibilityIdentifier = "History List"
    
    reloadData()
  }
  
  @objc func notificationReceived(_ notification: Notification) {
    switch notification.name {
    case .DynamicFontChanged:
      if emptyStateOverlayView.superview != nil {
        emptyStateOverlayView.removeFromSuperview()
      }
      emptyStateOverlayView = createEmptyStateOverview()
    default:
      // no need to do anything at all
      break
    }
  }
  
  override func reloadData() {
    guard let frc = frc else {
      return
    }
    
    do {
      try frc.performFetch()
    } catch let error as NSError {
      print(error.description)
    }
    
    tableView.reloadData()
    updateEmptyPanelState()
  }
  
  fileprivate func updateEmptyPanelState() {
    if frc?.fetchedObjects?.count == 0 {
      if self.emptyStateOverlayView.superview == nil {
        self.tableView.addSubview(self.emptyStateOverlayView)
        self.emptyStateOverlayView.snp.makeConstraints { make -> Void in
          make.edges.equalTo(self.tableView)
          make.size.equalTo(self.view)
        }
      }
    } else {
      self.emptyStateOverlayView.removeFromSuperview()
    }
  }
  
  fileprivate func createEmptyStateOverview() -> UIView {
    let overlayView = UIView()
    overlayView.backgroundColor = UIColor.white
    
    return overlayView
  }
  
  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = super.tableView(tableView, cellForRowAt: indexPath)
    configureCell(cell, atIndexPath: indexPath)
    return cell
  }
  
  func configureCell(_ _cell: UITableViewCell, atIndexPath indexPath: IndexPath) {
    guard let cell = _cell as? TwoLineTableViewCell else { return }
    
    if !tableView.isEditing {
      cell.gestureRecognizers?.forEach { cell.removeGestureRecognizer($0) }
      let lp = UILongPressGestureRecognizer(target: self, action: #selector(longPressedCell(_:)))
      cell.addGestureRecognizer(lp)
    }
    
    let site = frc!.object(at: indexPath)
    cell.backgroundColor = UIColor.clear
    cell.setLines(site.title, detailText: site.url)
    
    cell.imageView?.contentMode = .scaleAspectFit
    cell.imageView?.image = FaviconFetcher.defaultFavicon
    cell.imageView?.layer.cornerRadius = 6
    cell.imageView?.layer.masksToBounds = true
    
    if let faviconMO = site.domain?.favicon, let urlString = faviconMO.url, let url = URL(string: urlString), let siteUrlString = site.url, let siteUrl = URL(string: siteUrlString) {
      setCellImage(cell, iconUrl: url, cacheWithUrl: siteUrl)
    } else if let urlString = site.url, let siteUrl = URL(string: urlString) {
      if ImageCache.shared.hasImage(siteUrl, type: .square) {
        // no relationship - check cache for icon which may have been stored recently for url.
        ImageCache.shared.image(siteUrl, type: .square, callback: { (image) in
          DispatchQueue.main.async {
            cell.imageView?.image = image
          }
        })
      } else {
        // no relationship - attempt to resolove domain problem
        let context = DataController.viewContext
        if let domain = Domain.getOrCreateForUrl(siteUrl, context: context), let faviconMO = domain.favicon, let urlString = faviconMO.url, let url = URL(string: urlString) {
          DispatchQueue.main.async {
            self.setCellImage(cell, iconUrl: url, cacheWithUrl: siteUrl)
          }
        } else {
          // last resort - download the icon
          downloadFaviconsAndUpdateForUrl(siteUrl, indexPath: indexPath)
        }
      }
    }
  }
  
  fileprivate func downloadFaviconsAndUpdateForUrl(_ url: URL, indexPath: IndexPath) {
    weak var weakSelf = self
    FaviconFetcher.getForURL(url, profile: profile).uponQueue(DispatchQueue.main) { result in
      guard let favicons = result.successValue, favicons.count > 0, let foundIconUrl = favicons.first?.url.asURL, let cell = weakSelf?.tableView.cellForRow(at: indexPath) else { return }
      self.setCellImage(cell, iconUrl: foundIconUrl, cacheWithUrl: url)
    }
  }
  
  fileprivate func setCellImage(_ cell: UITableViewCell, iconUrl: URL, cacheWithUrl: URL) {
    ImageCache.shared.image(cacheWithUrl, type: .square, callback: { (image) in
      if image != nil {
        DispatchQueue.main.async {
          cell.imageView?.image = image
        }
      } else {
        DispatchQueue.main.async {
          cell.imageView?.sd_setImage(with: iconUrl, completed: { (img, err, type, url) in
            guard let img = img else {
              // avoid retrying to find an icon when none can be found, hack skips FaviconFetch
              ImageCache.shared.cache(FaviconFetcher.defaultFavicon, url: cacheWithUrl, type: .square, callback: nil)
              cell.imageView?.image = FaviconFetcher.defaultFavicon
              return
            }
            ImageCache.shared.cache(img, url: cacheWithUrl, type: .square, callback: nil)
          })
        }
      }
    })
  }
  
  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    let site = frc?.object(at: indexPath)
    
    if let u = site?.url, let url = URL(string: u) {
      linkNavigationDelegate?.linkNavigatorDidSelectURL(url: url, visitType: .typed)
    }
    tableView.deselectRow(at: indexPath, animated: true)
  }
  
  func numberOfSections(in tableView: UITableView) -> Int {
    return frc?.sections?.count ?? 0
  }
  
  func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
    guard let sections = frc?.sections else { return nil }
    return sections.indices ~= section ? sections[section].name : nil
  }
  
  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    guard let sections = frc?.sections else { return 0 }
    return sections.indices ~= section ? sections[section].numberOfObjects : 0
  }
  
  func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
    return true
  }
  
  func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCellEditingStyle, forRowAt indexPath: IndexPath) {
    if (editingStyle == UITableViewCellEditingStyle.delete) {
      if let obj = self.frc?.object(at: indexPath) as? History {
        obj.delete()
      }
    }
  }
}

extension HistoryViewController: NSFetchedResultsControllerDelegate {
  func controllerWillChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
    tableView.beginUpdates()
  }
  
  func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
    tableView.endUpdates()
  }
  
  func controller(_ controller: NSFetchedResultsController<NSFetchRequestResult>, didChange sectionInfo: NSFetchedResultsSectionInfo, atSectionIndex sectionIndex: Int, for type: NSFetchedResultsChangeType) {
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
  
  func controller(_ controller: NSFetchedResultsController<NSFetchRequestResult>, didChange anObject: Any, at indexPath: IndexPath?, for type: NSFetchedResultsChangeType, newIndexPath: IndexPath?) {
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
    }
    updateEmptyPanelState()
  }
}

private let ActionSheetTitleMaxLength = 120

extension HistoryViewController {
  
  @objc private func longPressedCell(_ gesture: UILongPressGestureRecognizer) {
    guard gesture.state == .began,
      let cell = gesture.view as? UITableViewCell,
      let indexPath = tableView.indexPath(for: cell),
      let history = frc?.object(at: indexPath) else {
        return
    }
    
    let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
    
    alert.title = history.url?.replacingOccurrences(of: "mailto:", with: "").ellipsize(maxLength: ActionSheetTitleMaxLength)
    actionsForHistory(history, currentTabIsPrivate: tabState.type.isPrivate).forEach { alert.addAction($0) }
    
    let cancelAction = UIAlertAction(title: Strings.Cancel, style: .cancel, handler: nil)
    alert.addAction(cancelAction)
    
    // If we're showing an arrow popup, set the anchor to the long press location.
    if let popoverPresentationController = alert.popoverPresentationController {
      popoverPresentationController.sourceView = view
      popoverPresentationController.sourceRect = CGRect(origin: gesture.location(in: view), size: CGSize(width: 0, height: 16))
      popoverPresentationController.permittedArrowDirections = .any
    }
    
    present(alert, animated: true)
  }
  
  private func actionsForHistory(_ history: History, currentTabIsPrivate: Bool) -> [UIAlertAction] {
    guard let urlString = history.url, let url = URL(string: urlString) else { return [] }
    
    var items: [UIAlertAction] = []
    // New Tab
    items.append(UIAlertAction(title: Strings.Open_In_Background_Tab, style: .default, handler: { [weak self] _ in
      guard let `self` = self else { return }
      self.linkNavigationDelegate?.linkNavigatorDidRequestToOpenInNewTab(url, isPrivate: currentTabIsPrivate)
    }))
    if !currentTabIsPrivate {
      // New Private Tab
      items.append(UIAlertAction(title: Strings.Open_In_New_Private_Tab, style: .default, handler: { [weak self] _ in
        guard let `self` = self else { return }
        self.linkNavigationDelegate?.linkNavigatorDidRequestToOpenInNewTab(url, isPrivate: true)
      }))
    }
    // Copy
    items.append(UIAlertAction(title: Strings.Copy_Link, style: .default, handler: { [weak self] _ in
      guard let `self` = self else { return }
      self.linkNavigationDelegate?.linkNavigatorDidRequestToCopyURL(url)
    }))
    // Share
    items.append(UIAlertAction(title: Strings.Share_Link, style: .default, handler: { [weak self] _ in
      guard let `self` = self else { return }
      self.linkNavigationDelegate?.linkNavigatorDidRequestToShareURL(url)
    }))
    
    return items
  }
}
