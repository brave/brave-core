/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import CoreData
import Shared
import XCGLogger
import Eureka
import Storage
import Data
import BraveShared

private let log = Logger.browserLogger

// MARK: - UX constants.

struct BookmarksViewControllerUX {
  fileprivate static let BookmarkFolderHeaderViewChevronInset: CGFloat = 10
  fileprivate static let BookmarkFolderChevronSize: CGFloat = 20
  fileprivate static let BookmarkFolderChevronLineWidth: CGFloat = 4.0
  fileprivate static let BookmarkFolderTextColor = UIColor(red: 92/255, green: 92/255, blue: 92/255, alpha: 1.0)
  fileprivate static let WelcomeScreenPadding: CGFloat = 15
  fileprivate static let WelcomeScreenItemTextColor = UIColor.gray
  fileprivate static let WelcomeScreenItemWidth = 170
  fileprivate static let SeparatorRowHeight: CGFloat = 0.5
}

class BookmarkEditingViewController: FormViewController {
  var completionBlock: ((_ controller: BookmarkEditingViewController) -> Void)?
  
  var folders: [Bookmark] = []
  
  var bookmarksPanel: BookmarksViewController!
  var bookmark: Bookmark!
  var bookmarkIndexPath: IndexPath!
  
  let BOOKMARK_TITLE_ROW_TAG: String = "BOOKMARK_TITLE_ROW_TAG"
  let BOOKMARK_URL_ROW_TAG: String = "BOOKMARK_URL_ROW_TAG"
  let BOOKMARK_FOLDER_ROW_TAG: String = "BOOKMARK_FOLDER_ROW_TAG"
  
  var titleRow:TextRow?
  var urlRow: URLRow?
  
  init(bookmarksPanel: BookmarksViewController, indexPath: IndexPath, bookmark: Bookmark) {
    super.init(nibName: nil, bundle: nil)
    
    self.bookmark = bookmark
    self.bookmarksPanel = bookmarksPanel
    self.bookmarkIndexPath = indexPath
    
    // get top-level folders
    folders = Bookmark.getFolders(bookmark: nil, context: DataController.viewContext)
  }
  
  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)
    //called when we're about to be popped, so use this for callback
    if let block = self.completionBlock {
      block(self)
    }
    
    self.bookmark.update(customTitle: self.titleRow?.value, url: self.urlRow?.value?.absoluteString, save: true)
  }
  
  var isEditingFolder: Bool {
    return bookmark.isFolder
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    let firstSectionName = !isEditingFolder ?  Strings.Bookmark_Info : Strings.Bookmark_Folder
    
    let nameSection = Section(firstSectionName)
    
    nameSection <<< TextRow() { row in
      row.tag = BOOKMARK_TITLE_ROW_TAG
      row.title = Strings.Name
      row.value = bookmark.displayTitle
      self.titleRow = row
    }
    
    form +++ nameSection
    
    // Only show URL option for bookmarks, not folders
    if !isEditingFolder {
      nameSection <<< URLRow() { row in
        row.tag = BOOKMARK_URL_ROW_TAG
        row.title = Strings.URL
        row.value = URL(string: bookmark.url ?? "")
        self.urlRow = row
      }
    }
    
    // Currently no way to edit bookmark/folder locations
    // See de9e1cc for removal of this logic
  }
}

class BookmarksViewController: SiteTableViewController, HomePanel {
  /// Called when the bookmarks are updated via some user input (i.e. Delete, edit, etc.)
  var bookmarksDidChange: (() -> Void)?
  
  weak var homePanelDelegate: HomePanelDelegate?
  
  var bookmarksFRC: NSFetchedResultsController<NSFetchRequestResult>?
  
  var editBookmarksToolbar: UIToolbar!
  var editBookmarksButton: UIBarButtonItem!
  var addFolderButton: UIBarButtonItem?
  weak var addBookmarksFolderOkAction: UIAlertAction?
  
  var isEditingIndividualBookmark:Bool = false
  
  var currentFolder: Bookmark? = nil
  let tabState: TabState
  
  init(folder: Bookmark?, tabState: TabState) {
    self.tabState = tabState
    
    super.init(nibName: nil, bundle: nil)
    
    self.currentFolder = folder
    self.title = folder?.displayTitle ?? Strings.Bookmarks
    self.bookmarksFRC = Bookmark.frc(parentFolder: folder)
    self.bookmarksFRC?.delegate = self
    
    // FIXME: NotificationMainThreadContextSignificantlyChanged is gone
  }
  
  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    self.view.backgroundColor = BraveUX.BackgroundColorForSideToolbars
    
    tableView.allowsSelectionDuringEditing = true
    
    let navBar = self.navigationController?.navigationBar
    navBar?.barTintColor = BraveUX.BackgroundColorForSideToolbars
    navBar?.isTranslucent = false
    navBar?.titleTextAttributes = [.font : UIFont.systemFont(ofSize: UIConstants.DefaultChromeSize, weight: .medium), .foregroundColor : BraveUX.GreyJ]
    navBar?.clipsToBounds = true
    
    let width = self.view.bounds.size.width
    let toolbarHeight = CGFloat(44)
    
    editBookmarksToolbar = UIToolbar(frame: CGRect(x: 0, y: 0, width: width, height: toolbarHeight))
    createEditBookmarksToolbar()
    editBookmarksToolbar.barTintColor = BraveUX.BackgroundColorForSideToolbars
    editBookmarksToolbar.isTranslucent = false
    
    self.view.addSubview(editBookmarksToolbar)
    
    editBookmarksToolbar.snp.makeConstraints { make in
      make.height.equalTo(toolbarHeight)
      make.left.equalTo(self.view)
      make.right.equalTo(self.view)
      make.bottom.equalTo(self.view.safeAreaLayoutGuide)
    }
    
    tableView.snp.makeConstraints { make in
      make.bottom.equalTo(self.view).inset(UIEdgeInsetsMake(0, 0, toolbarHeight, 0))
    }
    
    reloadData()
  }
  
  override func reloadData() {
    
    do {
      try self.bookmarksFRC?.performFetch()
    } catch let error as NSError {
      print(error.description)
    }
    
    super.reloadData()
  }
  
  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    
    reloadData()
  }
  
  func disableTableEditingMode() {
    switchTableEditingMode(true)
  }
  
  func switchTableEditingMode(_ forceOff:Bool = false) {
    let editMode:Bool = forceOff ? false : !tableView.isEditing
    tableView.setEditing(editMode, animated: forceOff ? false : true)
    
    updateEditBookmarksButton(editMode)
    resetCellLongpressGesture(tableView.isEditing)
    
    addFolderButton?.isEnabled = !editMode
  }
  
  func updateEditBookmarksButton(_ tableIsEditing:Bool) {
    self.editBookmarksButton.title = tableIsEditing ? Strings.Done : Strings.Edit
    self.editBookmarksButton.style = tableIsEditing ? .done : .plain
  }
  
  func resetCellLongpressGesture(_ editing: Bool) {
    for cell in self.tableView.visibleCells {
      cell.gestureRecognizers?.forEach { cell.removeGestureRecognizer($0) }
      if !editing {
        cell.addGestureRecognizer(UILongPressGestureRecognizer(target: self, action: #selector(longPressedCell(_:))))
      }
    }
  }
  
  func createEditBookmarksToolbar() {
    var items = [UIBarButtonItem]()
    
    items.append(UIBarButtonItem.fixedSpace(5))
    
    addFolderButton = UIBarButtonItem(image: UIImage(named: "bookmarks_newfolder_icon")?.withRenderingMode(.alwaysTemplate), style: .plain, target: self, action: #selector(onAddBookmarksFolderButton))
    items.append(addFolderButton!)
    
    items.append(UIBarButtonItem(barButtonSystemItem: .flexibleSpace, target: self, action: nil))
    
    editBookmarksButton = UIBarButtonItem(image: UIImage(named: "edit")?.withRenderingMode(.alwaysTemplate), style: .plain, target: self, action: #selector(onEditBookmarksButton))
    items.append(editBookmarksButton)
    items.append(UIBarButtonItem.fixedSpace(5))
    
    items.forEach { $0.tintColor = BraveUX.LightBlue }
    
    editBookmarksToolbar.items = items
    
    // This removes the small top border from the toolbar
    editBookmarksToolbar.clipsToBounds = true
  }
  
  func onDeleteBookmarksFolderButton() {
    guard let currentFolder = currentFolder else {
      NSLog("Delete folder button pressed but no folder object exists (probably at root), ignoring.")
      return
    }
    
    // TODO: Needs to be recursive
    currentFolder.remove(save: true)
    
    self.navigationController?.popViewController(animated: true)
  }
  
  @objc private func onAddBookmarksFolderButton() {
    let alert = UIAlertController.userTextInputAlert(title: Strings.NewFolder, message: Strings.EnterFolderName) {
      input, _ in
      if let input = input, !input.isEmpty {
        self.addFolder(titled: input)
      }
    }
    self.present(alert, animated: true) {}
  }
  
  func addFolder(titled title: String) {
    Bookmark.add(url: nil, title: nil, customTitle: title, parentFolder: currentFolder, isFolder: true)
    tableView.setContentOffset(CGPoint.zero, animated: true)
  }
  
  @objc private func onEditBookmarksButton() {
    switchTableEditingMode()
  }
  
  func tableView(_ tableView: UITableView, moveRowAt sourceIndexPath: IndexPath, to destinationIndexPath: IndexPath) {
    // Using the same reorder logic as in FavoritesDataSource
    Bookmark.reorderBookmarks(frc: bookmarksFRC, sourceIndexPath: sourceIndexPath, destinationIndexPath: destinationIndexPath)
  }
  
  func tableView(_ tableView: UITableView, canMoveRowAt indexPath: IndexPath) -> Bool {
    return true
  }
  
  func notificationReceived(_ notification: Notification) {
    switch notification.name {
    case NSNotification.Name.UITextFieldTextDidChange:
      if let okAction = addBookmarksFolderOkAction, let textField = notification.object as? UITextField {
        okAction.isEnabled = (textField.text?.count ?? 0) > 0
      }
      break
    default:
      // no need to do anything at all
      log.warning("Received unexpected notification \(notification.name)")
      break
    }
  }
  
  func currentBookmarksPanel() -> BookmarksViewController {
    guard let controllers = navigationController?.viewControllers.filter({ $0 as? BookmarksViewController != nil }) else {
      return self
    }
    return controllers.last as? BookmarksViewController ?? self
  }
  
  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    return bookmarksFRC?.fetchedObjects?.count ?? 0
  }
  
  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = super.tableView(tableView, cellForRowAt: indexPath)
    configureCell(cell, atIndexPath: indexPath)
    return cell
  }
  
//  override func getLongPressUrl(forIndexPath indexPath: IndexPath) -> (URL?, [Int]?) {
//    guard let obj = bookmarksFRC?.object(at: indexPath) as? Bookmark else { return (nil, nil) }
//    return (obj.url != nil ? URL(string: obj.url!) : nil, obj.isFolder ? obj.syncUUID : nil)
//  }
  
  fileprivate func configureCell(_ cell: UITableViewCell, atIndexPath indexPath: IndexPath) {
    
    guard let item = bookmarksFRC?.object(at: indexPath) as? Bookmark else { return }
    cell.tag = item.objectID.hashValue
    
    func configCell(image: UIImage? = nil, icon: FaviconMO? = nil) {
      if !tableView.isEditing {
        cell.gestureRecognizers?.forEach { cell.removeGestureRecognizer($0) }
        let lp = UILongPressGestureRecognizer(target: self, action: #selector(longPressedCell(_:)))
        cell.addGestureRecognizer(lp)
      }
      
      cell.imageView?.contentMode = .scaleAspectFit
      cell.imageView?.image = FaviconFetcher.defaultFavicon
      cell.imageView?.layer.cornerRadius = 6
      cell.imageView?.layer.masksToBounds = true
      
      if let image = image {
        // folder or preset icon
        cell.imageView?.image = image
        cell.imageView?.contentMode = .center
      }
      else if let faviconMO = item.domain?.favicon, let urlString = faviconMO.url, let url = URL(string: urlString), let bookmarkUrlString = item.url, let bookmarkUrl = URL(string: bookmarkUrlString) {
        // favicon object associated through domain relationship - set from cache or download
        setCellImage(cell, iconUrl: url, cacheWithUrl: bookmarkUrl)
      }
      else if let urlString = item.url, let bookmarkUrl = URL(string: urlString) {
        if ImageCache.shared.hasImage(bookmarkUrl, type: .square) {
          // no relationship - check cache for icon which may have been stored recently for url.
          ImageCache.shared.image(bookmarkUrl, type: .square, callback: { (image) in
            DispatchQueue.main.async {
              cell.imageView?.image = image
            }
          })
        }
        else {
          // no relationship - attempt to resolove domain problem
          let context = DataController.viewContext
          if let domain = Domain.getOrCreateForUrl(bookmarkUrl, context: context), let faviconMO = domain.favicon, let urlString = faviconMO.url, let url = URL(string: urlString) {
            DispatchQueue.main.async {
              self.setCellImage(cell, iconUrl: url, cacheWithUrl: bookmarkUrl)
            }
          }
          else {
            // last resort - download the icon
            downloadFaviconsAndUpdateForUrl(bookmarkUrl, indexPath: indexPath)
          }
        }
      }
    }
    
    let fontSize: CGFloat = 14.0
    cell.textLabel?.text = item.displayTitle ?? item.url
    cell.textLabel?.lineBreakMode = .byTruncatingTail
    
    cell.contentView.backgroundColor = .white
    
    if !item.isFolder {
      configCell(icon: item.domain?.favicon)
      cell.textLabel?.font = UIFont.systemFont(ofSize: fontSize)
      cell.accessoryType = .none
    } else {
      configCell(image: UIImage(named: "bookmarks_folder_hollow"))
      cell.textLabel?.font = UIFont.boldSystemFont(ofSize: fontSize)
      cell.accessoryType = .disclosureIndicator
      if let twoLineCell = cell as? TwoLineTableViewCell {
        twoLineCell.setRightBadge(nil)
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
      }
      else {
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
  
  override func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
    return nil
  }
  
  override func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
    return super.tableView(tableView, heightForRowAt: indexPath)
  }

  override func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
    return 0
  }
  
  func tableView(_ tableView: UITableView, willSelectRowAt indexPath: IndexPath) -> IndexPath? {
    return indexPath
  }
  
  func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
    guard let bookmark = bookmarksFRC?.object(at: indexPath) as? Bookmark else { return false }
    
    return !bookmark.isFavorite
  }
  
  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: false)
    
    guard let bookmark = bookmarksFRC?.object(at: indexPath) as? Bookmark else { return }
    
    if !bookmark.isFolder {
      if tableView.isEditing {
        //show editing view for bookmark item
        self.showEditBookmarkController(tableView, indexPath: indexPath)
      }
      else {
        if let url = URL(string: bookmark.url ?? "") {
          homePanelDelegate?.homePanel(self, didSelectURL: url, visitType: .bookmark)
        }
      }
    } else {
      if tableView.isEditing {
        //show editing view for bookmark item
        self.showEditBookmarkController(tableView, indexPath: indexPath)
      }
      else {
        let nextController = BookmarksViewController(folder: bookmark, tabState: tabState)
        nextController.profile = profile
        nextController.bookmarksDidChange = bookmarksDidChange
        nextController.homePanelDelegate = homePanelDelegate
        
        self.navigationController?.pushViewController(nextController, animated: true)
      }
    }
  }
  
  func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCellEditingStyle, forRowAt indexPath: IndexPath) {
    // Intentionally blank. Required to use UITableViewRowActions
  }
  
  func tableView(_ tableView: UITableView, editingStyleForRowAt indexPath: IndexPath) -> UITableViewCellEditingStyle {
    return .delete
  }
  
  func tableView(_ tableView: UITableView, editActionsForRowAt indexPath: IndexPath) -> [UITableViewRowAction]? {
    guard let item = bookmarksFRC?.object(at: indexPath) as? Bookmark else { return nil }
    
    let deleteAction = UITableViewRowAction(style: UITableViewRowActionStyle.destructive, title: Strings.Delete, handler: { (action, indexPath) in
      
      func delete() {
        item.remove(save: true)
      }
      
      if let children = item.children, !children.isEmpty {
        let alert = UIAlertController(title: "Delete Folder?", message: "This will delete all folders and bookmarks inside. Are you sure you want to continue?", preferredStyle: .alert)
        alert.addAction(UIAlertAction(title: "Cancel", style: UIAlertActionStyle.cancel, handler: nil))
        alert.addAction(UIAlertAction(title: "Yes, Delete", style: UIAlertActionStyle.destructive) { action in
          delete()
        })
        
        self.present(alert, animated: true, completion: nil)
      } else {
        delete()
      }
    })
    
    let editAction = UITableViewRowAction(style: UITableViewRowActionStyle.normal, title: Strings.Edit, handler: { (action, indexPath) in
      self.showEditBookmarkController(tableView, indexPath: indexPath)
    })
    
    return [deleteAction, editAction]
  }
  
  fileprivate func showEditBookmarkController(_ tableView: UITableView, indexPath:IndexPath) {
    guard let item = bookmarksFRC?.object(at: indexPath) as? Bookmark, !item.isFavorite else { return }
    let nextController = BookmarkEditingViewController(bookmarksPanel: self, indexPath: indexPath, bookmark: item)
    
    nextController.completionBlock = { controller in
      self.isEditingIndividualBookmark = false
    }
    self.isEditingIndividualBookmark = true
    self.navigationController?.pushViewController(nextController, animated: true)
  }
  
}

private protocol BookmarkFolderTableViewHeaderDelegate {
  func didSelectHeader()
}

extension BookmarksViewController: BookmarkFolderTableViewHeaderDelegate {
  fileprivate func didSelectHeader() {
    self.navigationController?.popViewController(animated: true)
  }
}

fileprivate class BookmarkFolderTableViewHeader : UITableViewHeaderFooterView {
  var delegate: BookmarkFolderTableViewHeaderDelegate?
  
  lazy var titleLabel: UILabel = {
    let label = UILabel()
    label.textColor = UIConstants.HighlightBlue
    return label
  }()
  
  lazy var chevron: ChevronView = {
    let chevron = ChevronView(direction: .left)
    chevron.tintColor = UIConstants.HighlightBlue
    chevron.lineWidth = BookmarksViewControllerUX.BookmarkFolderChevronLineWidth
    return chevron
  }()
  
  lazy var topBorder: UIView = {
    let view = UIView()
    view.backgroundColor = SiteTableViewControllerUX.HeaderBorderColor
    return view
  }()
  
  lazy var bottomBorder: UIView = {
    let view = UIView()
    view.backgroundColor = SiteTableViewControllerUX.HeaderBorderColor
    return view
  }()
  
  override var textLabel: UILabel? {
    return titleLabel
  }
  
  override init(reuseIdentifier: String?) {
    super.init(reuseIdentifier: reuseIdentifier)
    
    isUserInteractionEnabled = true
    
    let tapGestureRecognizer = UITapGestureRecognizer(target: self, action: #selector(BookmarkFolderTableViewHeader.viewWasTapped(_:)))
    tapGestureRecognizer.numberOfTapsRequired = 1
    addGestureRecognizer(tapGestureRecognizer)
    
    addSubview(topBorder)
    addSubview(bottomBorder)
    contentView.addSubview(chevron)
    contentView.addSubview(titleLabel)
    
    chevron.snp.makeConstraints { make in
      make.left.equalTo(contentView).offset(BookmarksViewControllerUX.BookmarkFolderHeaderViewChevronInset)
      make.centerY.equalTo(contentView)
      make.size.equalTo(BookmarksViewControllerUX.BookmarkFolderChevronSize)
    }
    
    titleLabel.snp.makeConstraints { make in
      make.left.equalTo(chevron.snp.right).offset(BookmarksViewControllerUX.BookmarkFolderHeaderViewChevronInset)
      make.right.greaterThanOrEqualTo(contentView).offset(-BookmarksViewControllerUX.BookmarkFolderHeaderViewChevronInset)
      make.centerY.equalTo(contentView)
    }
    
    topBorder.snp.makeConstraints { make in
      make.left.right.equalTo(self)
      make.top.equalTo(self).offset(-0.5)
      make.height.equalTo(0.5)
    }
    
    bottomBorder.snp.makeConstraints { make in
      make.left.right.bottom.equalTo(self)
      make.height.equalTo(0.5)
    }
  }
  
  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  @objc fileprivate func viewWasTapped(_ gestureRecognizer: UITapGestureRecognizer) {
    delegate?.didSelectHeader()
  }
}

extension BookmarksViewController : NSFetchedResultsControllerDelegate {
  func controllerWillChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
    tableView.beginUpdates()
  }
  
  func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
    tableView.endUpdates()
    bookmarksDidChange?()
  }
  
  func controller(_ controller: NSFetchedResultsController<NSFetchRequestResult>, didChange anObject: Any, at indexPath: IndexPath?, for type: NSFetchedResultsChangeType, newIndexPath: IndexPath?) {
    switch (type) {
    case .update:
      guard let indexPath = indexPath, let cell = tableView.cellForRow(at: indexPath) else {
        return
      }
      configureCell(cell, atIndexPath: indexPath)
    case .insert:
      guard let path = newIndexPath else {
        return
      }
      tableView.insertRows(at: [path], with: .automatic)
    case .delete:
      guard let indexPath = indexPath else {
        return
      }
      tableView.deleteRows(at: [indexPath], with: .automatic)
    case .move:
      break
    }
  }
}

private let ActionSheetTitleMaxLength = 120

extension BookmarksViewController {
  
  @objc private func longPressedCell(_ gesture: UILongPressGestureRecognizer) {
    guard gesture.state == .began,
      let cell = gesture.view as? UITableViewCell,
      let indexPath = tableView.indexPath(for: cell),
      let bookmark = bookmarksFRC?.object(at: indexPath) as? Bookmark else {
      return
    }
    
    let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
    
    if bookmark.isFolder {
      actionsForFolder(bookmark).forEach { alert.addAction($0) }
    } else {
      alert.title = bookmark.url?.replacingOccurrences(of: "mailto:", with: "").ellipsize(maxLength: ActionSheetTitleMaxLength)
      actionsForBookmark(bookmark, currentTabIsPrivate: tabState.isPrivate).forEach { alert.addAction($0) }
    }
    
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
  
  private func actionsForFolder(_ folder: Bookmark) -> [UIAlertAction] {
    let children = Bookmark.getChildren(forFolderUUID: folder.syncUUID, ignoreFolders: true) ?? []
    
    let urls: [URL] = children.compactMap { b in
      guard let url = b.url else { return nil }
      return URL(string: url)
    }
    
    return [
      UIAlertAction(
        title: String(format: Strings.Open_All_Bookmarks, children.count),
        style: .default,
        handler: { [weak self] _ in
          self?.homePanelDelegate?.homePanelDidRequestToBatchOpenURLs(urls)
        }
      )
    ]
  }
  
  private func actionsForBookmark(_ bookmark: Bookmark, currentTabIsPrivate: Bool) -> [UIAlertAction] {
    guard let urlString = bookmark.url, let url = URL(string: urlString) else { return [] }
    
    var items: [UIAlertAction] = []
    // New Tab
    items.append(UIAlertAction(title: Strings.Open_In_Background_Tab, style: .default, handler: { [weak self] _ in
      guard let `self` = self else { return }
      self.homePanelDelegate?.homePanelDidRequestToOpenInNewTab(url, isPrivate: currentTabIsPrivate)
    }))
    if !currentTabIsPrivate {
      // New Private Tab
      items.append(UIAlertAction(title: Strings.Open_In_New_Private_Tab, style: .default, handler: { [weak self] _ in
        guard let `self` = self else { return }
        self.homePanelDelegate?.homePanelDidRequestToOpenInNewTab(url, isPrivate: true)
      }))
    }
    // Copy
    items.append(UIAlertAction(title: Strings.Copy_Link, style: .default, handler: { [weak self] _ in
      guard let `self` = self else { return }
      self.homePanelDelegate?.homePanelDidRequestToCopyURL(url)
    }))
    // Share
    items.append(UIAlertAction(title: Strings.Share_Link, style: .default, handler: { [weak self] _ in
      guard let `self` = self else { return }
      self.homePanelDelegate?.homePanelDidRequestToShareURL(url)
    }))
    
    return items
  }
}
