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
  
  var titleRow: TextRow?
  var urlRow: URLRow?
  
  init(bookmarksPanel: BookmarksViewController, indexPath: IndexPath, bookmark: Bookmark) {
    super.init(nibName: nil, bundle: nil)
    
    self.bookmark = bookmark
    self.bookmarksPanel = bookmarksPanel
    self.bookmarkIndexPath = indexPath
    
    folders = Bookmark.getTopLevelFolders()
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
    
    self.bookmark.update(customTitle: self.titleRow?.value, url: self.urlRow?.value?.absoluteString)
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

class BookmarksViewController: SiteTableViewController {
  /// Called when the bookmarks are updated via some user input (i.e. Delete, edit, etc.)
  var bookmarksDidChange: (() -> Void)?
  
  weak var linkNavigationDelegate: LinkNavigationDelegate?
  
  var bookmarksFRC: NSFetchedResultsController<Bookmark>?
  
  var editBookmarksToolbar: UIToolbar!
  var editBookmarksButton: UIBarButtonItem!
  var addFolderButton: UIBarButtonItem?
  weak var addBookmarksFolderOkAction: UIAlertAction?
  
  var isEditingIndividualBookmark: Bool = false
  
  var currentFolder: Bookmark?
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
    navBar?.titleTextAttributes = [.font: UIFont.systemFont(ofSize: UIConstants.DefaultChromeSize, weight: .medium), .foregroundColor: BraveUX.GreyJ]
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
    
    tableView.contentInset = UIEdgeInsets(top: 0, left: 0, bottom: toolbarHeight, right: 0)
    tableView.scrollIndicatorInsets = tableView.contentInset
    
    reloadData()
  }
  
  override func reloadData() {
    
    do {
        // Recreate the frc if it was previously removed
        // (when user navigated into a nested folder for example)
        if bookmarksFRC == nil {
            bookmarksFRC = Bookmark.frc(parentFolder: currentFolder)
            bookmarksFRC?.delegate = self
        }
      try self.bookmarksFRC?.performFetch()
    } catch let error as NSError {
      log.error(error.description)
    }
    
    super.reloadData()
  }
  
  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    
    reloadData()
  }
    
    override func viewDidDisappear(_ animated: Bool) {
        super.viewDidDisappear(animated)
        // Make sure to remove fetch results controller when view disappears.
        // Otherwise, it may result in crash if a user is in a nested folder and
        // sync changes happen.
        bookmarksFRC = nil
    }
  
  func disableTableEditingMode() {
    switchTableEditingMode(true)
  }
  
  func switchTableEditingMode(_ forceOff: Bool = false) {
    let editMode: Bool = forceOff ? false : !tableView.isEditing
    tableView.setEditing(editMode, animated: forceOff ? false : true)
    
    updateEditBookmarksButton(editMode)
    resetCellLongpressGesture(tableView.isEditing)
    
    addFolderButton?.isEnabled = !editMode
  }
  
  func updateEditBookmarksButton(_ tableIsEditing: Bool) {
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
    
    addFolderButton = UIBarButtonItem(image: #imageLiteral(resourceName: "bookmarks_newfolder_icon").template, style: .plain, target: self, action: #selector(onAddBookmarksFolderButton))
    items.append(addFolderButton!)
    
    items.append(UIBarButtonItem(barButtonSystemItem: .flexibleSpace, target: self, action: nil))
    
    editBookmarksButton = UIBarButtonItem(image: #imageLiteral(resourceName: "edit").template, style: .plain, target: self, action: #selector(onEditBookmarksButton))
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
    currentFolder.delete()
    
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
    Bookmark.addFolder(title: title, parentFolder: currentFolder)
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
    case UITextField.textDidChangeNotification:
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
    
    // Make sure Bookmark at index path exists,
    // `frc.object(at:)` crashes otherwise, doesn't fail safely with nil
    if let objectsCount = bookmarksFRC?.fetchedObjects?.count, indexPath.row >= objectsCount {
        fatalError("Bookmarks FRC index out of bounds")
    }
    
    guard let item = bookmarksFRC?.object(at: indexPath) else { return }
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
        cell.imageView?.layer.borderWidth = 0.0
      } else {
        cell.imageView?.layer.borderColor = BraveUX.faviconBorderColor.cgColor
        cell.imageView?.layer.borderWidth = BraveUX.faviconBorderWidth
        // favicon object associated through domain relationship - set from cache or download
        cell.imageView?.setIconMO(item.domain?.favicon, forURL: URL(string: item.url ?? ""))
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
      configCell(image: #imageLiteral(resourceName: "bookmarks_folder_hollow"))
      cell.textLabel?.font = UIFont.boldSystemFont(ofSize: fontSize)
      cell.accessoryType = .disclosureIndicator
      if let twoLineCell = cell as? TwoLineTableViewCell {
        twoLineCell.setRightBadge(nil)
      }
    }
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
    guard let bookmark = bookmarksFRC?.object(at: indexPath) else { return false }
    
    return !bookmark.isFavorite
  }
  
  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: false)
    
    guard let bookmark = bookmarksFRC?.object(at: indexPath) else { return }
    
    if !bookmark.isFolder {
      if tableView.isEditing {
        //show editing view for bookmark item
        self.showEditBookmarkController(tableView, indexPath: indexPath)
      } else {
        if let url = URL(string: bookmark.url ?? "") {
          linkNavigationDelegate?.linkNavigatorDidSelectURL(url: url, visitType: .bookmark)
        }
      }
    } else {
      if tableView.isEditing {
        //show editing view for bookmark item
        self.showEditBookmarkController(tableView, indexPath: indexPath)
      } else {
        let nextController = BookmarksViewController(folder: bookmark, tabState: tabState)
        nextController.profile = profile
        nextController.bookmarksDidChange = bookmarksDidChange
        nextController.linkNavigationDelegate = linkNavigationDelegate
        
        self.navigationController?.pushViewController(nextController, animated: true)
      }
    }
  }
  
  func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
    // Intentionally blank. Required to use UITableViewRowActions
  }
  
  func tableView(_ tableView: UITableView, editingStyleForRowAt indexPath: IndexPath) -> UITableViewCell.EditingStyle {
    return .delete
  }
  
  func tableView(_ tableView: UITableView, editActionsForRowAt indexPath: IndexPath) -> [UITableViewRowAction]? {
    guard let item = bookmarksFRC?.object(at: indexPath) else { return nil }
    
    let deleteAction = UITableViewRowAction(style: UITableViewRowAction.Style.destructive, title: Strings.Delete,
                                            handler: { action, indexPath in
      
      if let children = item.children, !children.isEmpty {
        let alert = UIAlertController(title: Strings.DeleteBookmarksFolderAlertTitle, message: Strings.DeleteBookmarksFolderAlertMessage, preferredStyle: .alert)
        alert.addAction(UIAlertAction(title: Strings.CancelButtonTitle, style: .cancel))
        alert.addAction(UIAlertAction(title: Strings.YesDeleteButtonTitle, style: .destructive) { _ in
          item.delete()
        })
        
        self.present(alert, animated: true, completion: nil)
      } else {
        item.delete()
      }
    })
    
    let editAction = UITableViewRowAction(style: UITableViewRowAction.Style.normal, title: Strings.Edit, handler: { (action, indexPath) in
      self.showEditBookmarkController(tableView, indexPath: indexPath)
    })
    
    return [deleteAction, editAction]
  }
  
  fileprivate func showEditBookmarkController(_ tableView: UITableView, indexPath: IndexPath) {
    guard let item = bookmarksFRC?.object(at: indexPath), !item.isFavorite else { return }
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

fileprivate class BookmarkFolderTableViewHeader: UITableViewHeaderFooterView {
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

extension BookmarksViewController: NSFetchedResultsControllerDelegate {
  func controllerWillChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
    tableView.beginUpdates()
  }
  
  func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
    tableView.endUpdates()
    bookmarksDidChange?()
  }
  
  func controller(_ controller: NSFetchedResultsController<NSFetchRequestResult>, didChange anObject: Any, at indexPath: IndexPath?, for type: NSFetchedResultsChangeType, newIndexPath: IndexPath?) {
    switch type {
    case .update:
        let update = { (path: IndexPath?) in
            // When Bookmark is moved to another folder, it can be interpreted as update action
            // (since the object is not deleted but updated to have a different parent Bookmark)
            // Make sure we are not out of bounds here.
            if let path = path, let cell = self.tableView.cellForRow(at: path),
                let fetchedObjectsCount = self.bookmarksFRC?.fetchedObjects?.count, path.row < fetchedObjectsCount {
                    self.configureCell(cell, atIndexPath: path)
            }
        }
        [indexPath, newIndexPath].forEach(update)
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
      let bookmark = bookmarksFRC?.object(at: indexPath) else {
      return
    }
    
    let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
    
    if bookmark.isFolder {
      actionsForFolder(bookmark).forEach { alert.addAction($0) }
    } else {
      alert.title = bookmark.url?.replacingOccurrences(of: "mailto:", with: "").ellipsize(maxLength: ActionSheetTitleMaxLength)
      actionsForBookmark(bookmark, currentTabIsPrivate: tabState.type.isPrivate).forEach { alert.addAction($0) }
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
  
  private func actionsForFolder(_ folder: Bookmark) -> [UIAlertAction] {
    let children = Bookmark.getChildren(forFolder: folder, includeFolders: false) ?? []
    
    let urls: [URL] = children.compactMap { b in
      guard let url = b.url else { return nil }
      return URL(string: url)
    }
    
    return [
      UIAlertAction(
        title: String(format: Strings.Open_All_Bookmarks, children.count),
        style: .default,
        handler: { [weak self] _ in
          self?.linkNavigationDelegate?.linkNavigatorDidRequestToBatchOpenURLs(urls)
        }
      )
    ]
  }
  
  private func actionsForBookmark(_ bookmark: Bookmark, currentTabIsPrivate: Bool) -> [UIAlertAction] {
    guard let urlString = bookmark.url, let url = URL(string: urlString) else { return [] }
    
    var items: [UIAlertAction] = []
    // New Tab
    items.append(UIAlertAction(title: Strings.OpenNewTabButtonTitle, style: .default, handler: { [weak self] _ in
      guard let `self` = self else { return }
      self.linkNavigationDelegate?.linkNavigatorDidRequestToOpenInNewTab(url, isPrivate: currentTabIsPrivate)
    }))
    if !currentTabIsPrivate {
      // New Private Tab
      items.append(UIAlertAction(title: Strings.OpenNewPrivateTabButtonTitle, style: .default, handler: { [weak self] _ in
        guard let `self` = self else { return }
        self.linkNavigationDelegate?.linkNavigatorDidRequestToOpenInNewTab(url, isPrivate: true)
      }))
    }
    // Copy
    items.append(UIAlertAction(title: Strings.CopyLinkActionTitle, style: .default, handler: { [weak self] _ in
      guard let `self` = self else { return }
      self.linkNavigationDelegate?.linkNavigatorDidRequestToCopyURL(url)
    }))
    // Share
    items.append(UIAlertAction(title: Strings.ShareLinkActionTitle, style: .default, handler: { [weak self] _ in
      guard let `self` = self else { return }
      self.linkNavigationDelegate?.linkNavigatorDidRequestToShareURL(url)
    }))
    
    return items
  }
}
