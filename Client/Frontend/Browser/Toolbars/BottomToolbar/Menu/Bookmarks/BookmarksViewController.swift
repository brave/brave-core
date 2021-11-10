/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import CoreData
import Shared
import Data
import BraveShared
import CoreServices

private let log = Logger.browserLogger

class BookmarksViewController: SiteTableViewController, ToolbarUrlActionsProtocol {

    private var bookmarksFRC: BookmarksV2FetchResultsController?
    private let bookmarkManager: BookmarkManager
    /// Called when the bookmarks are updated via some user input (i.e. Delete, edit, etc.)
    private var bookmarksDidChange: (() -> Void)?
    weak var toolbarUrlActionsDelegate: ToolbarUrlActionsDelegate?
    
    private lazy var editBookmarksButton: UIBarButtonItem? = UIBarButtonItem().then {
        $0.image = #imageLiteral(resourceName: "edit").template
        $0.style = .plain
        $0.target = self
        $0.action = #selector(onEditBookmarksButton)
    }
    
    private lazy var addFolderButton: UIBarButtonItem? = UIBarButtonItem().then {
        $0.image = #imageLiteral(resourceName: "bookmarks_newfolder_icon").template
        $0.style = .plain
        $0.target = self
        $0.action = #selector(onAddBookmarksFolderButton)
    }
    
    private lazy var importExportButton: UIBarButtonItem? = UIBarButtonItem().then {
        $0.image = #imageLiteral(resourceName: "nav-share").template
        $0.style = .plain
        $0.target = self
        $0.action = #selector(importExportAction(_:))
    }
    
    private let spinner = UIActivityIndicatorView().then {
        $0.snp.makeConstraints { make in
            make.size.equalTo(24)
        }
        $0.hidesWhenStopped = true
        $0.isHidden = true
    }
    
    private var leftToolbarItems: [UIBarButtonItem?] {
        var items: [UIBarButtonItem?] = [.fixedSpace(5)]
        if currentFolder == nil {
            items.append(importExportButton)
            
            // Unlike Chromium, old CoreData implementation did not have permanent folders
            if !Preferences.Chromium.syncV2BookmarksMigrationCompleted.value {
                items.append(.fixedSpace(16))
                items.append(addFolderButton)
            }
        } else {
            items.append(addFolderButton)
        }
        
        return items
    }
    
    private var isLoading = false {
        didSet {
            if isLoading {
                view.addSubview(spinner)
                spinner.snp.makeConstraints {
                    $0.center.equalTo(view.snp.center)
                }
                
                spinner.startAnimating()
                spinner.isHidden = false
            } else {
                spinner.stopAnimating()
                spinner.removeFromSuperview()
            }
        }
    }
    
    private weak var addBookmarksFolderOkAction: UIAlertAction?
    
    private var isEditingIndividualBookmark = false
    
    private var currentFolder: Bookmarkv2?
    
    /// Certain bookmark actions are different in private browsing mode.
    private let isPrivateBrowsing: Bool
    
    private var isAtBookmarkRootLevel: Bool {
        return self.currentFolder == nil
    }
    
    private let importExportUtility = BraveCoreImportExportUtility()
    private var documentInteractionController: UIDocumentInteractionController?
    
    private var searchBookmarksTimer: Timer?
    private var isBookmarksBeingSearched = false
    private let bookmarksSearchController = UISearchController(searchResultsController: nil)
    private var bookmarksSearchQuery = ""
    private lazy var noSearchResultOverlayView = createNoSearchResultOverlayView()

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
            $0.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(tappedDone))
        }
        
        navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(tappedDone))
        
        tableView.do {
            $0.allowsSelectionDuringEditing = true
            $0.register(BookmarkTableViewCell.self,
                           forCellReuseIdentifier: String(describing: BookmarkTableViewCell.self))
        }
    }
    
    override func accessibilityPerformEscape() -> Bool {
        dismiss(animated: true)
        return true
    }
    
    private func setUpToolbar() {
        let flexibleSpace = UIBarButtonItem(barButtonSystemItem: .flexibleSpace, target: self, action: nil)
        
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

                let nextController = BookmarksViewController(folder: self.currentFolder?.parent, bookmarkManager: self.bookmarkManager, isPrivateBrowsing: self.isPrivateBrowsing)
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
    
    private func createNoSearchResultOverlayView() -> UIView {
        let overlayView = UIView().then {
            $0.backgroundColor = .secondaryBraveBackground
        }
        
        let welcomeLabel = UILabel().then {
            $0.text = Strings.noSearchResultsfound
            $0.textAlignment = .center
            $0.font = DynamicFontHelper.defaultHelper.DeviceFontLight
            $0.textColor = .braveLabel
            $0.numberOfLines = 0
            $0.adjustsFontSizeToFitWidth = true
        }
        
        overlayView.addSubview(welcomeLabel)
        
        welcomeLabel.snp.makeConstraints { make in
            make.centerX.equalTo(overlayView)
            // Sets proper top constraint for iPhone 6 in portait and for iPad.
            make.centerY.equalTo(overlayView).offset(-180).priority(100)
            // Sets proper top constraint for iPhone 4, 5 in portrait.
            make.top.greaterThanOrEqualTo(overlayView).offset(50)
            make.width.equalTo(170)
        }
        
        return overlayView
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
        let alert = UIAlertController.userTextInputAlert(title: Strings.newFolder, message: Strings.enterFolderName) {
            input, _ in
            if let input = input, !input.isEmpty {
                self.addFolder(titled: input)
            }
        }
        self.present(alert, animated: true) {}
    }
    
    @objc private func importExportAction(_ sender: UIBarButtonItem) {
        let alert = AlertController(title: nil, message: nil, preferredStyle: .actionSheet)
        alert.popoverPresentationController?.barButtonItem = sender
        let importAction = UIAlertAction(title: Strings.bookmarksImportAction, style: .default) { [weak self] _ in
            let vc = UIDocumentPickerViewController(documentTypes: [String(kUTTypeHTML)], in: .import)
            vc.delegate = self
            self?.present(vc, animated: true)
        }
        
        let exportAction = UIAlertAction(title: Strings.bookmarksExportAction, style: .default) { [weak self] _ in
            let fileUrl = FileManager.default.temporaryDirectory.appendingPathComponent("Bookmarks").appendingPathExtension("html")
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
    
    @objc private func longPressedCell(_ gesture: UILongPressGestureRecognizer) {
        guard gesture.state == .began,
              let cell = gesture.view as? UITableViewCell,
              let indexPath = tableView.indexPath(for: cell),
              let bookmark = fetchBookmarkItem(at: indexPath) else {
            return
        }
                        
        presentLongPressActions(gesture, urlString: bookmark.url, isPrivateBrowsing: isPrivateBrowsing,
                                customActions: bookmark.isFolder ? folderLongPressActions(bookmark) : nil)
    }
    
    private func folderLongPressActions(_ folder: Bookmarkv2) -> [UIAlertAction] {
        let children = bookmarkManager.getChildren(forFolder: folder, includeFolders: false) ?? []
        
        let urls: [URL] = children.compactMap { b in
            guard let url = b.url else { return nil }
            return URL(string: url)
        }
        
        return [
            UIAlertAction(
                title: String(format: Strings.openAllBookmarks, children.count),
                style: .default,
                handler: { [weak self] _ in
                    self?.toolbarUrlActionsDelegate?.batchOpen(urls)
                    self?.presentingViewController?.dismiss(animated: true)
                }
            )
        ]
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
            log.error(error.description)
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
    
    //MARK: Internal
    
    private func disableTableEditingMode() {
        switchTableEditingMode(true)
    }
    
    private func switchTableEditingMode(_ forceOff: Bool = false) {
        let editMode: Bool = forceOff ? false : !tableView.isEditing
        tableView.setEditing(editMode, animated: forceOff ? false : true)
        
        updateEditBookmarksButton(editMode)
        resetCellLongpressGesture(tableView.isEditing)
        
        editBookmarksButton?.isEnabled = bookmarksFRC?.fetchedObjectsCount != 0
        addFolderButton?.isEnabled = !editMode
    }
    
    private func updateEditBookmarksButton(_ tableIsEditing: Bool) {
        self.editBookmarksButton?.title = tableIsEditing ? Strings.done : Strings.edit
        self.editBookmarksButton?.style = tableIsEditing ? .done : .plain
    }
    
    private func resetCellLongpressGesture(_ editing: Bool) {
        for cell in self.tableView.visibleCells {
            cell.gestureRecognizers?.forEach { cell.removeGestureRecognizer($0) }
            if !editing {
                cell.addGestureRecognizer(UILongPressGestureRecognizer(target: self, action: #selector(longPressedCell(_:))))
            }
        }
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
        
        func configCell(image: UIImage? = nil, icon: FaviconMO? = nil) {
            if !tableView.isEditing {
                cell.gestureRecognizers?.forEach { cell.removeGestureRecognizer($0) }
                let lp = UILongPressGestureRecognizer(target: self, action: #selector(longPressedCell(_:)))
                cell.addGestureRecognizer(lp)
            }
            
            if !shouldReuse {
                return
            }
            
            cell.imageView?.cancelFaviconLoad()
            cell.backgroundColor = .clear
            cell.imageView?.contentMode = .scaleAspectFit
            cell.imageView?.image = FaviconFetcher.defaultFaviconImage
            cell.imageView?.layer.cornerRadius = 6
            cell.imageView?.layer.cornerCurve = .continuous
            cell.imageView?.layer.masksToBounds = true
            
            if let image = image {
                // folder or preset icon
                cell.imageView?.image = image
                cell.imageView?.contentMode = .center
                cell.imageView?.layer.borderWidth = 0.0
                cell.imageView?.clearMonogramFavicon()
            } else {
                cell.imageView?.layer.borderColor = BraveUX.faviconBorderColor.cgColor
                cell.imageView?.layer.borderWidth = BraveUX.faviconBorderWidth
                
                // Sets the favIcon of a cell's imageView from Brave-Core
                // If the icon does not exist, fallback to our FavIconFetcher
                let setFavIcon = { (cell: UITableViewCell, item: Bookmarkv2) in
                    cell.imageView?.clearMonogramFavicon()
                    
                    if let icon = item.bookmarkNode.icon {
                        cell.imageView?.image = icon
                    } else if let domain = item.domain, let url = domain.url?.asURL {
                        // favicon object associated through domain relationship - set from cache only
                        cell.imageView?.loadFavicon(for: url, domain: domain, fallbackMonogramCharacter: item.title?.first, cachedOnly: true)
                    } else {
                        cell.imageView?.clearMonogramFavicon()
                        cell.imageView?.image = FaviconFetcher.defaultFaviconImage
                    }
                }
                
                // Brave-Core favIcons are async and notify an observer when changed..
                bookmarkManager.addFavIconObserver(item) { [weak item] in
                    guard let item = item else { return }
                    
                    setFavIcon(cell, item)
                }
                
                // `item.icon` triggers a favIcon load on Brave-Core, then it will notify observers
                // and update `item.isFavIconLoading` and `item.isFavIconLoaded` properties..
                // Order of this if-statement matters because of that logic!
                if (item.bookmarkNode.icon == nil && (item.bookmarkNode.isFavIconLoading || item.bookmarkNode.isFavIconLoaded))
                    || item.bookmarkNode.icon != nil {
                    setFavIcon(cell, item)
                } else if let domain = item.domain, let url = domain.url?.asURL {
                    // favicon object associated through domain relationship - set from cache or download
                    cell.imageView?.loadFavicon(for: url, domain: domain, fallbackMonogramCharacter: item.title?.first)
                } else {
                    cell.imageView?.clearMonogramFavicon()
                    cell.imageView?.image = FaviconFetcher.defaultFaviconImage
                }
            }
        }
        
        let fontSize: CGFloat = 14.0
        cell.textLabel?.text = item.title ?? item.url
        cell.textLabel?.lineBreakMode = .byTruncatingTail
        
        if !item.isFolder {
            configCell(icon: item.domain?.favicon)
            cell.textLabel?.font = UIFont.systemFont(ofSize: fontSize)
            cell.accessoryType = .none
        } else {
            configCell(image: #imageLiteral(resourceName: "bookmarks_folder_hollow").withTintColor(.braveLabel))
            cell.textLabel?.font = UIFont.boldSystemFont(ofSize: fontSize)
            cell.accessoryType = .disclosureIndicator
            cell.setRightBadge(nil)
        }
    }
        
    override func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
        0
    }
    
    override func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
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
                        /// Donate Custom Intent Open Bookmark List
                        if !self.isPrivateBrowsing {
                            ActivityShortcutManager.shared.donateCustomIntent(for: .openBookmarks, with: url.absoluteString)
                        }
                        
                        self.toolbarUrlActionsDelegate?.select(url: url, visitType: .bookmark)
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

                let nextController = BookmarksViewController(folder: bookmark, bookmarkManager: bookmarkManager, isPrivateBrowsing: isPrivateBrowsing)
                nextController.profile = profile
                nextController.bookmarksDidChange = bookmarksDidChange
                nextController.toolbarUrlActionsDelegate = toolbarUrlActionsDelegate
                self.navigationController?.pushViewController(nextController, animated: true)
            }
        }
    }
    
    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        isBookmarksBeingSearched ? bookmarkManager.fetchedSearchObjectsCount : bookmarksFRC?.fetchedObjectsCount ?? 0
    }
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        guard let cell = tableView
                .dequeueReusableCell(withIdentifier: String(describing: BookmarkTableViewCell.self), for: indexPath) as? BookmarkTableViewCell else {
            assertionFailure()
            return UITableViewCell()
        }
        
        configureCell(cell, atIndexPath: indexPath)
        return cell
    }
    
    override func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
        var fetchedBookmarkItem: Bookmarkv2?
        if isBookmarksBeingSearched {
            return true
        } else {
            fetchedBookmarkItem = bookmarksFRC?.object(at: indexPath)
            return fetchedBookmarkItem?.canBeDeleted ?? false

        }
    }
}

// MARK: UITableViewDelegate - Editing

extension BookmarksViewController {
    
    func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
        // Intentionally blank. Required to use UITableViewRowActions
    }
    
    func tableView(_ tableView: UITableView, editingStyleForRowAt indexPath: IndexPath) -> UITableViewCell.EditingStyle {
        .delete
    }
    
    func tableView(_ tableView: UITableView, trailingSwipeActionsConfigurationForRowAt indexPath: IndexPath) -> UISwipeActionsConfiguration? {
        guard let item = fetchBookmarkItem(at: indexPath), item.canBeDeleted else { return nil }

        let deleteAction = UIContextualAction(style: .destructive, title: Strings.delete) { [weak self] _, _, completion in
            guard let self = self else {
                completion(false)
                return
            }
            
            if let children = item.children, !children.isEmpty {
                let alert = UIAlertController(title: Strings.deleteBookmarksFolderAlertTitle, message: Strings.deleteBookmarksFolderAlertMessage, preferredStyle: .alert)
                alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel) { _ in
                    completion(false)
                })
                alert.addAction(UIAlertAction(title: Strings.yesDeleteButtonTitle, style: .destructive) { _ in
                    self.bookmarkManager.delete(item)
                    completion(true)
                })
                
                self.present(alert, animated: true, completion: nil)
            } else {
                self.bookmarkManager.delete(item)
                completion(true)
            }
        }
        
        let editAction = UIContextualAction(style: .normal, title: Strings.edit) { [weak self] _, _, completion in
            self?.showEditBookmarkController(bookmark: item)
            completion(true)
        }
        
        return UISwipeActionsConfiguration(actions: [deleteAction, editAction])
    }
    
    func tableView(_ tableView: UITableView, moveRowAt sourceIndexPath: IndexPath, to destinationIndexPath: IndexPath) {
        guard let bookmarksFRC = bookmarksFRC else {
            return
        }

        bookmarkManager.reorderBookmarks(frc: bookmarksFRC, sourceIndexPath: sourceIndexPath, destinationIndexPath: destinationIndexPath)
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
            let vc = AddEditBookmarkTableViewController(bookmarkManager: bookmarkManager, mode: mode)
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
    
    func controller(_ controller: BookmarksV2FetchResultsController, didChange anObject: Any, at indexPath: IndexPath?, for type: NSFetchedResultsChangeType, newIndexPath: IndexPath?) {
        switch type {
        case .update:
            let update = { (path: IndexPath?) in
                // When Bookmark is moved to another folder, it can be interpreted as update action
                // (since the object is not deleted but updated to have a different parent Bookmark)
                // Make sure we are not out of bounds here.
                if let path = path,
                   let cell = self.tableView
                    .dequeueReusableCell(withIdentifier: String(describing: BookmarkTableViewCell.self),
                                         for: path) as? BookmarkTableViewCell,
                   let fetchedObjectsCount = self.bookmarksFRC?.fetchedObjectsCount, path.row < fetchedObjectsCount {
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

extension BookmarksViewController: UIDocumentPickerDelegate, UIDocumentInteractionControllerDelegate {
    
    func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) {
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
            try? FileManager.default.removeItem(at: url)
        }
        self.documentInteractionController = nil
    }
    
    func documentInteractionControllerDidDismissOptionsMenu(_ controller: UIDocumentInteractionController) {
        if let url = controller.url {
            try? FileManager.default.removeItem(at: url)
        }
        self.documentInteractionController = nil
    }
    
    func documentInteractionControllerDidDismissOpenInMenu(_ controller: UIDocumentInteractionController) {
        if let url = controller.url {
            try? FileManager.default.removeItem(at: url)
        }
        self.documentInteractionController = nil
    }
}

// MARK: Export-Import Bookmarks

extension BookmarksViewController {
    
    func importBookmarks(from url: URL) {
        isLoading = true
        
        self.importExportUtility.importBookmarks(from: url) { [weak self] success in
            guard let self = self else { return }
            
            self.isLoading = false
            
            let alert = UIAlertController(title: Strings.Sync.bookmarksImportPopupErrorTitle,
                                          message: success ? Strings.Sync.bookmarksImportPopupSuccessMessage :
                                            Strings.Sync.bookmarksImportPopupFailureMessage,
                                          preferredStyle: .alert)
            alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))
            self.present(alert, animated: true, completion: nil)
        }
    }
    
    func exportBookmarks(to url: URL) {
        isLoading = true
        
        self.importExportUtility.exportBookmarks(to: url) { [weak self] success in
            guard let self = self else { return }
            
            self.isLoading = false
            
            // Controller must be retained otherwise `AirDrop` and other sharing options will fail!
            self.documentInteractionController = UIDocumentInteractionController(url: url)
            guard let vc = self.documentInteractionController else { return }
            vc.uti = String(kUTTypeHTML)
            vc.name = "Bookmarks.html"
            vc.delegate = self
            
            guard let importExportButton = self.importExportButton else { return }
            vc.presentOptionsMenu(from: importExportButton, animated: true)
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
            Timer.scheduledTimer(timeInterval: 0.1, target: self, selector: #selector(fetchSearchResults(timer:)), userInfo: query, repeats: false)
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
