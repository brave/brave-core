// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import CoreData
import CoreServices
import Data
import Favicon
import Preferences
import Shared
import UIKit
import UniformTypeIdentifiers
import os.log

class BookmarksViewController: SiteTableViewController, ToolbarUrlActionsProtocol {

  weak var toolbarUrlActionsDelegate: ToolbarUrlActionsDelegate?
  private weak var addBookmarksFolderOkAction: UIAlertAction?

  private lazy var editBookmarksButton: UIBarButtonItem? = UIBarButtonItem().then {
    $0.image = UIImage(braveSystemNamed: "leo.edit.pencil")
    $0.style = .plain
    $0.target = self
    $0.action = #selector(onEditBookmarksButton)
  }

  private lazy var addFolderButton: UIBarButtonItem? = UIBarButtonItem().then {
    $0.image = UIImage(braveSystemNamed: "leo.folder.new")
    $0.style = .plain
    $0.target = self
    $0.action = #selector(onAddBookmarksFolderButton)
  }

  private lazy var importExportButton: UIBarButtonItem? = UIBarButtonItem().then {
    $0.image = UIImage(braveSystemNamed: "leo.share.macos")
    $0.style = .plain
    $0.target = self
    $0.action = #selector(importExportAction(_:))
  }

  private var leftToolbarItems: [UIBarButtonItem?] {
    var items: [UIBarButtonItem?] = [.fixedSpace(5)]
    if currentFolder == nil {
      items.append(importExportButton)
    } else {
      items.append(addFolderButton)
    }

    return items
  }

  private var bookmarksFRC: BookmarksV2FetchResultsController?
  private let bookmarkManager: BookmarkManager
  /// Called when the bookmarks are updated via some user input (i.e. Delete, edit, etc.)
  private var bookmarksDidChange: (() -> Void)?

  private var currentFolder: Bookmarkv2?

  /// Certain bookmark actions are different in private browsing mode.
  private let isPrivateBrowsing: Bool
  private var isEditingIndividualBookmark = false
  private var isAtBookmarkRootLevel: Bool {
    return self.currentFolder == nil
  }

  private let importExportUtility = BookmarksImportExportUtility()
  private var documentInteractionController: UIDocumentInteractionController?

  private var searchBookmarksTimer: Timer?
  private var isBookmarksBeingSearched = false
  private let bookmarksSearchController = UISearchController(searchResultsController: nil)
  private var bookmarksSearchQuery = ""
  private lazy var noSearchResultOverlayView = EmptyStateOverlayView(
    overlayDetails: EmptyOverlayStateDetails(title: Strings.noSearchResultsfound)
  )

  private var bookmarksExportSuccessful = false

  // MARK: Lifecycle

  init(folder: Bookmarkv2?, bookmarkManager: BookmarkManager, isPrivateBrowsing: Bool) {
    self.isPrivateBrowsing = isPrivateBrowsing
    self.bookmarkManager = bookmarkManager
    super.init(nibName: nil, bundle: nil)

    self.currentFolder = folder
    self.title = folder?.title ?? Strings.bookmarks
    self.bookmarksFRC = bookmarkManager.frc(parent: folder)
    self.bookmarksFRC?.delegate = self
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    applyTheme()
    setUpToolbar()
    updateEditBookmarksButtonStatus()
    updatedFolderHierarchy()
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)

    view.addSubview(spinner)
    spinner.snp.makeConstraints {
      $0.center.equalTo(self.view.snp.center)
    }
    spinner.startAnimating()
    spinner.isHidden = false
    updateLastVisitedFolder(currentFolder)

    bookmarkManager.waitForBookmarkModelLoaded({ [weak self] in
      guard let self = self else { return }

      self.navigationController?.setToolbarHidden(false, animated: true)
      self.reloadData()
      self.switchTableEditingMode(true)
      self.spinner.stopAnimating()
      self.spinner.removeFromSuperview()
      self.updateLastVisitedFolder(self.currentFolder)
    })
  }

  override func viewDidDisappear(_ animated: Bool) {
    super.viewDidDisappear(animated)
    // Make sure to remove fetch results controller when view disappears.
    // Otherwise, it may result in crash if a user is in a nested folder and
    // sync changes happen.
    bookmarksFRC = nil
  }

  // MARK: Layout - Theme

  private func applyTheme() {
    bookmarksSearchController.do {
      $0.searchBar.autocapitalizationType = .none
      $0.searchResultsUpdater = self
      $0.obscuresBackgroundDuringPresentation = false
      $0.searchBar.placeholder = Strings.searchBookmarksTitle
      $0.delegate = self
      $0.hidesNavigationBarDuringPresentation = true
    }

    navigationItem.do {
      $0.searchController = bookmarksSearchController
      $0.hidesSearchBarWhenScrolling = false
      $0.rightBarButtonItem = UIBarButtonItem(
        barButtonSystemItem: .done,
        target: self,
        action: #selector(tappedDone)
      )
    }

    navigationItem.rightBarButtonItem = UIBarButtonItem(
      barButtonSystemItem: .done,
      target: self,
      action: #selector(tappedDone)
    )

    tableView.do {
      $0.allowsSelectionDuringEditing = true
      $0.register(
        BookmarkTableViewCell.self,
        forCellReuseIdentifier: String(describing: BookmarkTableViewCell.self)
      )
    }

    definesPresentationContext = true
  }

  override func accessibilityPerformEscape() -> Bool {
    dismiss(animated: true)
    return true
  }

  private func setUpToolbar() {
    let flexibleSpace = UIBarButtonItem(
      barButtonSystemItem: .flexibleSpace,
      target: self,
      action: nil
    )

    let rightItem = { () -> UIBarButtonItem? in
      return currentFolder == nil ? nil : editBookmarksButton
    }()

    let items = (leftToolbarItems + [flexibleSpace, rightItem, .fixedSpace(5)]).compactMap { $0 }
    setToolbarItems(items, animated: true)
  }

  private func updateEditBookmarksButtonStatus() {
    guard let objectsCount = bookmarksFRC?.fetchedObjectsCount else { return }

    editBookmarksButton?.isEnabled = objectsCount != 0
    if tableView.isEditing && objectsCount == 0 {
      disableTableEditingMode()
    }
  }

  private func updatedFolderHierarchy() {
    DispatchQueue.main.async {
      guard let navigationController = self.navigationController else { return }
      let index = navigationController.viewControllers.firstIndex(of: self) ?? 0
      if index <= 0 && self.currentFolder != nil {

        let nextController = BookmarksViewController(
          folder: self.currentFolder?.parent,
          bookmarkManager: self.bookmarkManager,
          isPrivateBrowsing: self.isPrivateBrowsing
        )
        nextController.profile = self.profile
        nextController.bookmarksDidChange = self.bookmarksDidChange
        nextController.toolbarUrlActionsDelegate = self.toolbarUrlActionsDelegate
        navigationController.viewControllers.insert(nextController, at: index)
        nextController.loadViewIfNeeded()
      }
    }
  }

  private func updateLastVisitedFolder(_ folder: Bookmarkv2?) {
    Preferences.Chromium.lastBookmarksFolderNodeId.value = folder?.objectID ?? -1
  }

  private func updateEmptyPanelState() {
    if isBookmarksBeingSearched, bookmarkManager.fetchedSearchObjectsCount == 0 {
      showEmptyPanelState()
    } else {
      noSearchResultOverlayView.removeFromSuperview()
    }
  }

  private func showEmptyPanelState() {
    if noSearchResultOverlayView.superview == nil {
      view.addSubview(noSearchResultOverlayView)
      view.bringSubviewToFront(noSearchResultOverlayView)
      noSearchResultOverlayView.snp.makeConstraints { make -> Void in
        make.edges.equalTo(tableView)
      }
    }
  }

  // MARK: Actions

  @objc private func tappedDone() {
    dismiss(animated: true)
  }

  @objc private func onAddBookmarksFolderButton() {
    let alert = UIAlertController.userTextInputAlert(
      title: Strings.newFolder,
      message: Strings.enterFolderName
    ) {
      input,
      _ in
      if let input = input, !input.isEmpty {
        self.addFolder(titled: input)
      }
    }
    self.present(alert, animated: true) {}
  }

  @objc private func importExportAction(_ sender: UIBarButtonItem) {
    let alert = AlertController(title: nil, message: nil, preferredStyle: .actionSheet)
    alert.popoverPresentationController?.barButtonItem = sender
    let importAction = UIAlertAction(title: Strings.bookmarksImportAction, style: .default) {
      [weak self] _ in
      let vc = UIDocumentPickerViewController(forOpeningContentTypes: [.html, .zip])
      vc.delegate = self
      self?.present(vc, animated: true)
    }

    let exportAction = UIAlertAction(title: Strings.bookmarksExportAction, style: .default) {
      [weak self] _ in
      let fileUrl = FileManager.default.temporaryDirectory.appendingPathComponent("Bookmarks")
        .appendingPathExtension("html")
      self?.exportBookmarks(to: fileUrl)
    }

    let cancelAction = UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel)

    alert.addAction(importAction)
    alert.addAction(exportAction)
    alert.addAction(cancelAction)

    present(alert, animated: true)
  }

  @objc private func onEditBookmarksButton() {
    switchTableEditingMode()
  }

  // MARK: Data Fetch

  override func reloadData() {
    performBookmarkFetch()
    super.reloadData()
  }

  private func performBookmarkFetch() {
    if isBookmarksBeingSearched {
      return
    }

    do {
      // Recreate the frc if it was previously removed
      // (when user navigated into a nested folder for example)
      if bookmarksFRC == nil {
        bookmarksFRC = bookmarkManager.frc(parent: currentFolder)
        bookmarksFRC?.delegate = self
      }
      try self.bookmarksFRC?.performFetch()
    } catch let error as NSError {
      Logger.module.error("\(error.description)")
    }
  }

  private func refreshBookmarkSearchResult(with query: String = "") {
    guard isBookmarksBeingSearched else {
      return
    }

    isLoading = true

    fetchBookmarks(with: query) { [weak self] in
      self?.isLoading = false
    }
  }

  private func fetchBookmarks(with query: String, _ completion: @escaping () -> Void) {
    bookmarkManager.fetchBookmarks(with: query) { [weak self] in
      guard let self = self else { return }

      self.tableView.reloadData()
      self.updateEmptyPanelState()

      completion()
    }
  }

  private func fetchBookmarkItem(at indexPath: IndexPath) -> Bookmarkv2? {
    if isBookmarksBeingSearched {
      return bookmarkManager.searchObject(at: indexPath)
    } else {
      return bookmarksFRC?.object(at: indexPath)
    }
  }

  // MARK: Internal

  private func disableTableEditingMode() {
    switchTableEditingMode(true)
  }

  private func switchTableEditingMode(_ forceOff: Bool = false) {
    let editMode: Bool = forceOff ? false : !tableView.isEditing
    tableView.setEditing(editMode, animated: forceOff ? false : true)

    updateEditBookmarksButton(editMode)

    editBookmarksButton?.isEnabled = bookmarksFRC?.fetchedObjectsCount != 0
    addFolderButton?.isEnabled = !editMode
  }

  private func updateEditBookmarksButton(_ tableIsEditing: Bool) {
    self.editBookmarksButton?.title = tableIsEditing ? Strings.done : Strings.edit
    self.editBookmarksButton?.style = tableIsEditing ? .done : .plain
  }

  private func addFolder(titled title: String) {
    bookmarkManager.addFolder(title: title, parentFolder: currentFolder)
    tableView.setContentOffset(CGPoint.zero, animated: true)
  }

  fileprivate func configureCell(_ cell: BookmarkTableViewCell, atIndexPath indexPath: IndexPath) {
    var fetchedBookmarkItem: Bookmarkv2?

    if isBookmarksBeingSearched {
      fetchedBookmarkItem = bookmarkManager.searchObject(at: indexPath)
    } else {
      // Make sure Bookmark at index path exists,
      // `frc.object(at:)` crashes otherwise, doesn't fail safely with nil
      if let objectsCount = bookmarksFRC?.fetchedObjectsCount, indexPath.row >= objectsCount {
        assertionFailure("Bookmarks FRC index out of bounds")
        return
      }

      fetchedBookmarkItem = bookmarksFRC?.object(at: indexPath)
    }

    guard let item = fetchedBookmarkItem else { return }
    cell.tag = item.objectID

    // See if the cell holds the same bookmark. If yes, we do not have to recreate its image view
    // This makes scrolling through bookmarks better if there's many bookmarks with the same url
    let domainOrFolderName = item.isFolder ? item.title : (item.domain?.url ?? item.url)
    let shouldReuse = domainOrFolderName != cell.domainOrFolderName

    cell.domainOrFolderName = domainOrFolderName

    func configCell(image: UIImage? = nil) {
      if !tableView.isEditing {
        cell.gestureRecognizers?.forEach { cell.removeGestureRecognizer($0) }
      }

      if !shouldReuse {
        return
      }

      cell.imageView?.cancelFaviconLoad()
      cell.backgroundColor = .clear
      cell.imageView?.contentMode = .scaleAspectFit
      cell.imageView?.image = Favicon.defaultImage
      cell.imageView?.layer.cornerRadius = 6
      cell.imageView?.layer.cornerCurve = .continuous
      cell.imageView?.layer.masksToBounds = true
      cell.imageView?.tintColor = .braveLabel

      if let image = image {
        // folder or preset icon
        cell.imageView?.image = image
        cell.imageView?.contentMode = .center
        cell.imageView?.layer.borderWidth = 0.0
        cell.imageView?.clearMonogramFavicon()
      } else {
        cell.imageView?.layer.borderColor = FaviconUX.faviconBorderColor.cgColor
        cell.imageView?.layer.borderWidth = FaviconUX.faviconBorderWidth

        // Sets the favIcon of a cell's imageView from Brave-Core
        // If the icon does not exist, fallback to our FavIconFetcher
        func setFavicon(cell: UITableViewCell, item: Bookmarkv2) {
          cell.imageView?.clearMonogramFavicon()

          if let urlString = item.url, let url = URL(string: urlString) {
            cell.imageView?.loadFavicon(for: url, isPrivateBrowsing: isPrivateBrowsing) {
              [weak cell] favicon in
              if favicon?.isMonogramImage == true, let icon = item.bookmarkNode.icon {
                cell?.imageView?.image = icon
              }
            }
          } else if let icon = item.bookmarkNode.icon {
            cell.imageView?.image = icon
          } else {
            cell.imageView?.clearMonogramFavicon()
            cell.imageView?.image = Favicon.defaultImage
          }
        }

        // Brave-Core favIcons are async and notify an observer when changed..
        bookmarkManager.addFavIconObserver(item) { [weak item] in
          guard let item = item else { return }
          setFavicon(cell: cell, item: item)
        }

        setFavicon(cell: cell, item: item)
      }
    }

    let fontSize: CGFloat = 14.0
    cell.textLabel?.text = item.title ?? item.url
    cell.textLabel?.lineBreakMode = .byTruncatingTail
    cell.detailTextLabel?.lineBreakMode = .byTruncatingTail

    if !item.isFolder {
      configCell()
      cell.textLabel?.font = UIFont.systemFont(ofSize: fontSize)
      cell.accessoryType = .none
      cell.detailTextLabel?.text = nil
    } else {
      configCell(image: UIImage(braveSystemNamed: "leo.folder"))
      cell.textLabel?.font = UIFont.boldSystemFont(ofSize: fontSize)
      cell.accessoryType = .disclosureIndicator
      cell.setRightBadge(nil)
    }
  }

  override func tableView(
    _ tableView: UITableView,
    heightForHeaderInSection section: Int
  ) -> CGFloat {
    0
  }

  override func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView?
  {
    nil
  }

  func tableView(_ tableView: UITableView, willSelectRowAt indexPath: IndexPath) -> IndexPath? {
    indexPath
  }

  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: false)

    guard let bookmark = fetchBookmarkItem(at: indexPath) else { return }

    if !bookmark.isFolder {
      if tableView.isEditing {
        // show editing view for bookmark item
        self.showEditBookmarkController(bookmark: bookmark)
      } else {
        if let url = URL(string: bookmark.url ?? "") {
          let bookmarkClickEvent: (() -> Void)? = {
            // Donate Custom Intent Open Bookmark List
            if !self.isPrivateBrowsing {
              ActivityShortcutManager.shared.donateCustomIntent(
                for: .openBookmarks,
                with: url.absoluteString
              )
            }

            self.toolbarUrlActionsDelegate?.select(url: url, isUserDefinedURLNavigation: true)
          }

          if presentingViewController is MenuViewController {
            presentingViewController?.dismiss(animated: true, completion: bookmarkClickEvent)
          } else {
            dismiss(animated: true, completion: bookmarkClickEvent)
          }
        }
      }
    } else {
      if tableView.isEditing {
        // show editing view for bookmark item
        self.showEditBookmarkController(bookmark: bookmark)
      } else {
        self.updateLastVisitedFolder(bookmark)

        let nextController = BookmarksViewController(
          folder: bookmark,
          bookmarkManager: bookmarkManager,
          isPrivateBrowsing: isPrivateBrowsing
        )
        nextController.profile = profile
        nextController.bookmarksDidChange = bookmarksDidChange
        nextController.toolbarUrlActionsDelegate = toolbarUrlActionsDelegate
        self.navigationController?.pushViewController(nextController, animated: true)
      }
    }
  }

  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    isBookmarksBeingSearched
      ? bookmarkManager.fetchedSearchObjectsCount : bookmarksFRC?.fetchedObjectsCount ?? 0
  }

  override func tableView(
    _ tableView: UITableView,
    cellForRowAt indexPath: IndexPath
  ) -> UITableViewCell {
    guard
      let cell =
        tableView
        .dequeueReusableCell(
          withIdentifier: String(describing: BookmarkTableViewCell.self),
          for: indexPath
        ) as? BookmarkTableViewCell
    else {
      assertionFailure()
      return UITableViewCell()
    }

    configureCell(cell, atIndexPath: indexPath)
    return cell
  }

  override func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
    if isBookmarksBeingSearched {
      return true
    } else {
      if let fetchedBookmarkItem = bookmarksFRC?.object(at: indexPath),
        fetchedBookmarkItem.bookmarkNode.isValid
      {
        return fetchedBookmarkItem.canBeDeleted
      }
      return false
    }
  }

  func tableView(
    _ tableView: UITableView,
    contextMenuConfigurationForRowAt indexPath: IndexPath,
    point: CGPoint
  ) -> UIContextMenuConfiguration? {
    guard let bookmarkItem = fetchBookmarkItem(at: indexPath) else {
      return nil
    }

    var actionItemsMenu: UIMenu

    if bookmarkItem.isFolder {
      var actionChildren: [UIAction] = []
      let urls: [URL] = self.getAllURLS(forFolder: bookmarkItem)

      if urls.count > 0 {
        let openBatchURLAction = UIAction(
          title: String(format: Strings.openAllBookmarks, urls.count),
          image: UIImage(systemName: "arrow.up.forward.app"),
          handler: UIAction.deferredActionHandler { _ in
            self.toolbarUrlActionsDelegate?.batchOpen(urls)
          }
        )
        actionChildren.append(openBatchURLAction)
      }

      actionItemsMenu = UIMenu(title: "", identifier: nil, children: actionChildren)
    } else {
      guard let bookmarkItemURL = URL(string: bookmarkItem.url ?? "") else { return nil }

      let openInNewTabAction = UIAction(
        title: Strings.openNewTabButtonTitle,
        image: UIImage(systemName: "plus.square.on.square"),
        handler: UIAction.deferredActionHandler { [unowned self] _ in
          self.toolbarUrlActionsDelegate?.openInNewTab(
            bookmarkItemURL,
            isPrivate: isPrivateBrowsing
          )
          parent?.presentingViewController?.dismiss(animated: true)
        }
      )

      let newPrivateTabAction = UIAction(
        title: Strings.openNewPrivateTabButtonTitle,
        image: UIImage(systemName: "plus.square.fill.on.square.fill"),
        handler: UIAction.deferredActionHandler { [unowned self] _ in
          if !isPrivateBrowsing, Preferences.Privacy.privateBrowsingLock.value {
            self.askForLocalAuthentication { [weak self] success, error in
              if success {
                self?.toolbarUrlActionsDelegate?.openInNewTab(bookmarkItemURL, isPrivate: true)
                self?.parent?.presentingViewController?.dismiss(animated: true)
              }
            }
          } else {
            self.toolbarUrlActionsDelegate?.openInNewTab(bookmarkItemURL, isPrivate: true)
            parent?.presentingViewController?.dismiss(animated: true)
          }
        }
      )

      let copyAction = UIAction(
        title: Strings.copyLinkActionTitle,
        image: UIImage(systemName: "doc.on.doc"),
        handler: UIAction.deferredActionHandler { [unowned self] _ in
          self.toolbarUrlActionsDelegate?.copy(bookmarkItemURL)
        }
      )

      let shareAction = UIAction(
        title: Strings.shareLinkActionTitle,
        image: UIImage(systemName: "square.and.arrow.up"),
        handler: UIAction.deferredActionHandler { [unowned self] _ in
          self.toolbarUrlActionsDelegate?.share(bookmarkItemURL)
        }
      )

      var newTabActionMenu: [UIAction] = [openInNewTabAction]

      if !isPrivateBrowsing {
        newTabActionMenu.append(newPrivateTabAction)
      }

      let urlMenu = UIMenu(title: "", options: .displayInline, children: newTabActionMenu)
      let linkMenu = UIMenu(
        title: "",
        options: .displayInline,
        children: [copyAction, shareAction]
      )

      actionItemsMenu = UIMenu(
        title: bookmarkItemURL.absoluteString,
        identifier: nil,
        children: [urlMenu, linkMenu]
      )
    }

    return UIContextMenuConfiguration(identifier: indexPath as NSCopying, previewProvider: nil) {
      _ in
      return actionItemsMenu
    }
  }

  private func getAllURLS(forFolder rootFolder: Bookmarkv2) -> [URL] {
    var urls: [URL] = []
    guard rootFolder.isFolder else { return urls }
    var children = bookmarkManager.getChildren(forFolder: rootFolder, includeFolders: true) ?? []

    while !children.isEmpty {
      let bookmarkItem = children.removeFirst()
      if bookmarkItem.isFolder {
        // Follow the order of bookmark manager
        children.insert(
          contentsOf: bookmarkManager.getChildren(forFolder: bookmarkItem, includeFolders: true)
            ?? [],
          at: 0
        )
      } else if let bookmarkItemURL = URL(string: bookmarkItem.url ?? "") {
        urls.append(bookmarkItemURL)
      }
    }
    return urls
  }
}

// MARK: UITableViewDelegate - Editing

extension BookmarksViewController {

  func tableView(
    _ tableView: UITableView,
    commit editingStyle: UITableViewCell.EditingStyle,
    forRowAt indexPath: IndexPath
  ) {
    // Intentionally blank. Required to use UITableViewRowActions
  }

  func tableView(
    _ tableView: UITableView,
    editingStyleForRowAt indexPath: IndexPath
  ) -> UITableViewCell.EditingStyle {
    .delete
  }

  func tableView(
    _ tableView: UITableView,
    trailingSwipeActionsConfigurationForRowAt indexPath: IndexPath
  ) -> UISwipeActionsConfiguration? {
    guard let item = fetchBookmarkItem(at: indexPath), item.canBeDeleted else { return nil }

    let deleteAction = UIContextualAction(style: .destructive, title: Strings.delete) {
      [weak self] _, _, completion in
      guard let self = self else {
        completion(false)
        return
      }

      if let children = item.children, !children.isEmpty {
        let alert = UIAlertController(
          title: Strings.deleteBookmarksFolderAlertTitle,
          message: Strings.deleteBookmarksFolderAlertMessage,
          preferredStyle: .alert
        )
        alert.addAction(
          UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel) { _ in
            completion(false)
          }
        )
        alert.addAction(
          UIAlertAction(title: Strings.yesDeleteButtonTitle, style: .destructive) { _ in
            self.bookmarkManager.delete(item)
            completion(true)
          }
        )

        self.present(alert, animated: true, completion: nil)
      } else {
        self.bookmarkManager.delete(item)
        completion(true)
      }
    }

    let editAction = UIContextualAction(style: .normal, title: Strings.edit) {
      [weak self] _, _, completion in
      self?.showEditBookmarkController(bookmark: item)
      completion(true)
    }

    return UISwipeActionsConfiguration(actions: [deleteAction, editAction])
  }

  func tableView(
    _ tableView: UITableView,
    moveRowAt sourceIndexPath: IndexPath,
    to destinationIndexPath: IndexPath
  ) {
    guard let bookmarksFRC = bookmarksFRC else {
      return
    }

    bookmarkManager.reorderBookmarks(
      frc: bookmarksFRC,
      sourceIndexPath: sourceIndexPath,
      destinationIndexPath: destinationIndexPath
    )
  }

  func tableView(_ tableView: UITableView, canMoveRowAt indexPath: IndexPath) -> Bool {
    !isBookmarksBeingSearched
  }

  private func showEditBookmarkController(bookmark: Bookmarkv2) {
    self.isEditingIndividualBookmark = true

    var mode: BookmarkEditMode?
    if bookmark.isFolder {
      mode = .editFolder(bookmark)
    } else {
      mode = .editBookmark(bookmark)
    }

    if let mode = mode {
      let vc = AddEditBookmarkTableViewController(
        bookmarkManager: bookmarkManager,
        mode: mode,
        isPrivateBrowsing: isPrivateBrowsing
      )
      self.navigationController?.pushViewController(vc, animated: true)
    }
  }

}

// MARK: BookmarksV2FetchResultsDelegate

extension BookmarksViewController: BookmarksV2FetchResultsDelegate {

  func controllerWillChangeContent(_ controller: BookmarksV2FetchResultsController) {
    tableView.beginUpdates()
  }

  func controllerDidChangeContent(_ controller: BookmarksV2FetchResultsController) {
    tableView.endUpdates()
    bookmarksDidChange?()
    updateEditBookmarksButtonStatus()
  }

  func controller(
    _ controller: BookmarksV2FetchResultsController,
    didChange anObject: Any,
    at indexPath: IndexPath?,
    for type: NSFetchedResultsChangeType,
    newIndexPath: IndexPath?
  ) {
    switch type {
    case .update:
      let update = { (path: IndexPath?) in
        // When Bookmark is moved to another folder, it can be interpreted as update action
        // (since the object is not deleted but updated to have a different parent Bookmark)
        // Make sure we are not out of bounds here.
        if let path = path,
          let cell = self.tableView
            .dequeueReusableCell(
              withIdentifier: String(describing: BookmarkTableViewCell.self),
              for: path
            ) as? BookmarkTableViewCell,
          let fetchedObjectsCount = self.bookmarksFRC?.fetchedObjectsCount,
          path.row < fetchedObjectsCount
        {
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

  func controllerDidReloadContents(_ controller: BookmarksV2FetchResultsController) {
    if isBookmarksBeingSearched {
      refreshBookmarkSearchResult(with: bookmarksSearchQuery)
      return
    }

    // We're in some sort of invalid state in sync..
    // Somehow this folder was deleted but the user is currently viewing it..
    // Might be a good idea to let the user know in the future that the folder they are currently viewing
    // has been deleted from the sync chain on another device..
    // This is only possible if the user tries to purposely break sync..
    // See brave-ios/issues/3011 && brave-browser/issues/12530
    // - Brandon T.
    if let currentFolder = currentFolder, !currentFolder.existsInPersistentStore() {
      self.navigationController?.popToRootViewController(animated: true)
      return
    }

    // Everything is normal, we can reload the current view..
    reloadData()
  }
}

// MARK: UIDocumentPickerDelegate - UIDocumentInteractionControllerDelegate

extension BookmarksViewController: UIDocumentPickerDelegate, UIDocumentInteractionControllerDelegate
{

  func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL])
  {
    guard let url = urls.first, urls.count == 1 else {
      return
    }

    DispatchQueue.main.async {
      self.importBookmarks(from: url)
      self.documentInteractionController = nil
    }
  }

  func documentPickerWasCancelled(_ controller: UIDocumentPickerViewController) {
    self.documentInteractionController = nil
  }

  func documentInteractionControllerDidEndPreview(_ controller: UIDocumentInteractionController) {
    if let url = controller.url {
      Task {
        try await AsyncFileManager.default.removeItem(at: url)
      }
    }
    self.documentInteractionController = nil
  }

  func documentInteractionControllerDidDismissOptionsMenu(
    _ controller: UIDocumentInteractionController
  ) {
    if let url = controller.url {
      Task {
        try await AsyncFileManager.default.removeItem(at: url)
      }
    }
    self.documentInteractionController = nil

    if bookmarksExportSuccessful {
      bookmarksExportSuccessful = false

      let alert = UIAlertController(
        title: Strings.Sync.bookmarksImportExportPopupTitle,
        message: Strings.Sync.bookmarksExportPopupSuccessMessage,
        preferredStyle: .alert
      )
      alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))
      self.present(alert, animated: true, completion: nil)
    }
  }

  func documentInteractionControllerDidDismissOpenInMenu(
    _ controller: UIDocumentInteractionController
  ) {
    if let url = controller.url {
      Task {
        try await AsyncFileManager.default.removeItem(at: url)
      }
    }
    self.documentInteractionController = nil
  }
}

// MARK: Export-Import Bookmarks

extension BookmarksViewController {

  func importBookmarks(from url: URL) {
    isLoading = true

    Task { @MainActor in
      let success = await self.importExportUtility.importBookmarks(from: url)
      self.isLoading = false

      let alert = UIAlertController(
        title: Strings.Sync.bookmarksImportExportPopupTitle,
        message: success
          ? Strings.Sync.bookmarksImportPopupSuccessMessage
          : Strings.Sync.bookmarksImportPopupFailureMessage,
        preferredStyle: .alert
      )
      alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))
      self.present(alert, animated: true, completion: nil)
    }
  }

  func exportBookmarks(to url: URL) {
    isLoading = true

    Task { @MainActor in
      let success = await self.importExportUtility.exportBookmarks(to: url)

      self.isLoading = false

      if success {
        self.bookmarksExportSuccessful = true

        // Controller must be retained otherwise `AirDrop` and other sharing options will fail!
        self.documentInteractionController = UIDocumentInteractionController(url: url)
        guard let vc = self.documentInteractionController else { return }
        vc.uti = UTType.html.identifier
        vc.name = "Bookmarks.html"
        vc.delegate = self

        guard let importExportButton = self.importExportButton else { return }
        vc.presentOptionsMenu(from: importExportButton, animated: true)
      } else {
        let alert = UIAlertController(
          title: Strings.Sync.bookmarksImportExportPopupTitle,
          message: Strings.Sync.bookmarksExportPopupFailureMessage,
          preferredStyle: .alert
        )
        alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))
        self.present(alert, animated: true, completion: nil)
      }
    }
  }

}

// MARK: UISearchResultUpdating

extension BookmarksViewController: UISearchResultsUpdating {

  func updateSearchResults(for searchController: UISearchController) {
    guard let query = searchController.searchBar.text else { return }

    if searchBookmarksTimer != nil {
      searchBookmarksTimer?.invalidate()
      searchBookmarksTimer = nil
    }

    searchBookmarksTimer =
      Timer.scheduledTimer(
        timeInterval: 0.1,
        target: self,
        selector: #selector(fetchSearchResults(timer:)),
        userInfo: query,
        repeats: false
      )
  }

  @objc private func fetchSearchResults(timer: Timer) {
    guard let query = timer.userInfo as? String else {
      bookmarksSearchQuery = ""
      return
    }

    bookmarksSearchQuery = query
    refreshBookmarkSearchResult(with: bookmarksSearchQuery)
  }
}

// MARK: UISearchControllerDelegate

extension BookmarksViewController: UISearchControllerDelegate {

  func willPresentSearchController(_ searchController: UISearchController) {
    isBookmarksBeingSearched = true
    bookmarksSearchQuery = ""
    tableView.setEditing(false, animated: true)
    tableView.reloadData()

    // Bottom toolbar needs to be hidden when searching for bookmarks
    navigationController?.setToolbarHidden(true, animated: true)
  }

  func willDismissSearchController(_ searchController: UISearchController) {
    isBookmarksBeingSearched = false
    updateEmptyPanelState()
    reloadData()

    // Re-enable bottom var options when search is done
    navigationController?.setToolbarHidden(false, animated: true)
  }
}
