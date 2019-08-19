/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import CoreData
import Shared
import Eureka
import Data
import BraveShared

private let log = Logger.browserLogger

class BookmarksViewController: SiteTableViewController, ToolbarUrlActionsProtocol {
  /// Called when the bookmarks are updated via some user input (i.e. Delete, edit, etc.)
  var bookmarksDidChange: (() -> Void)?
  
  weak var toolbarUrlActionsDelegate: ToolbarUrlActionsDelegate?
  
  var bookmarksFRC: NSFetchedResultsController<Bookmark>?
  
    lazy var editBookmarksButton = UIBarButtonItem().then {
        $0.image = #imageLiteral(resourceName: "edit").template
        $0.style = .plain
        $0.tintColor = BraveUX.LightBlue
        $0.target = self
        $0.action = #selector(onEditBookmarksButton)
    }
    
    lazy var addFolderButton =  UIBarButtonItem().then {
        $0.image = #imageLiteral(resourceName: "bookmarks_newfolder_icon").template
        $0.style = .plain
        $0.tintColor = BraveUX.LightBlue
        $0.target = self
        $0.action = #selector(onAddBookmarksFolderButton)
    }
  weak var addBookmarksFolderOkAction: UIAlertAction?
  
  var isEditingIndividualBookmark: Bool = false
  
  var currentFolder: Bookmark?
    /// Certain bookmark actions are different in private browsing mode.
    let isPrivateBrowsing: Bool
  
    init(folder: Bookmark?, isPrivateBrowsing: Bool) {
    self.isPrivateBrowsing = isPrivateBrowsing
    
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
    
    setUpToolbar()
    updateEditBookmarksButtonStatus()
  }
    
    private func updateEditBookmarksButtonStatus() {
        guard let count = bookmarksFRC?.fetchedObjects?.count else { return }
        
        editBookmarksButton.isEnabled = count != 0
        if tableView.isEditing && count == 0 {
            disableTableEditingMode()
        }
    }
    
    private func setUpToolbar() {
        var padding: UIBarButtonItem { return UIBarButtonItem.fixedSpace(5) }
        let flexibleSpace = UIBarButtonItem(barButtonSystemItem: .flexibleSpace, target: self, action: nil)
        
        let items = [padding, addFolderButton, flexibleSpace, editBookmarksButton, padding]
        setToolbarItems(items, animated: true)
        
        navigationController?.toolbar.do {
            $0.barTintColor = BraveUX.BackgroundColorForSideToolbars
            $0.isTranslucent = false
        }
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
    navigationController?.setToolbarHidden(false, animated: true)
    reloadData()
    switchTableEditingMode(true)
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
    
    editBookmarksButton.isEnabled = bookmarksFRC?.fetchedObjects?.count != 0
    addFolderButton.isEnabled = !editMode
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
    
    @objc private func longPressedCell(_ gesture: UILongPressGestureRecognizer) {
        guard gesture.state == .began,
            let cell = gesture.view as? UITableViewCell,
            let indexPath = tableView.indexPath(for: cell),
            let bookmark = bookmarksFRC?.object(at: indexPath) else {
                return
        }
        
        presentLongPressActions(gesture, urlString: bookmark.url, isPrivateBrowsing: isPrivateBrowsing,
                                customActions: bookmark.isFolder ? folderLongPressActions(bookmark) : nil)
    }
    
    private func folderLongPressActions(_ folder: Bookmark) -> [UIAlertAction] {
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
                    self?.toolbarUrlActionsDelegate?.batchOpen(urls)
                    self?.dismiss(animated: true)
                }
            )
        ]
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
            dismiss(animated: true) {
                self.toolbarUrlActionsDelegate?.select(url: url, visitType: .bookmark)
            }
        }
      }
    } else {
      if tableView.isEditing {
        //show editing view for bookmark item
        self.showEditBookmarkController(tableView, indexPath: indexPath)
      } else {
        let nextController = BookmarksViewController(folder: bookmark, isPrivateBrowsing: isPrivateBrowsing)
        nextController.profile = profile
        nextController.bookmarksDidChange = bookmarksDidChange
        nextController.toolbarUrlActionsDelegate = toolbarUrlActionsDelegate
        
        // Show `Done` button on nested folder levels.
        nextController.navigationItem.setRightBarButton(navigationItem.rightBarButtonItem, animated: true)
        
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
    
    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return bookmarksFRC?.fetchedObjects?.count ?? 0
    }
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = super.tableView(tableView, cellForRowAt: indexPath)
        configureCell(cell, atIndexPath: indexPath)
        return cell
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

    self.isEditingIndividualBookmark = true
    
    let mode: BookmarkEditMode = item.isFolder ? .editFolder(item) : .editBookmark(item)
    
    let vc = AddEditBookmarkTableViewController(mode: mode)    
    self.navigationController?.pushViewController(vc, animated: true)
  }
  
}

extension BookmarksViewController: NSFetchedResultsControllerDelegate {
  func controllerWillChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
    tableView.beginUpdates()
  }
  
  func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
    tableView.endUpdates()
    bookmarksDidChange?()
    updateEditBookmarksButtonStatus()
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
    @unknown default:
        assertionFailure()
        break
    }
  }
}

