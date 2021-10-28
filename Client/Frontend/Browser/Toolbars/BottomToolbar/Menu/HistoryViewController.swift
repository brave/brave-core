/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import BraveShared
import Storage
import Data
import CoreData
import BraveCore

class HistoryViewController: SiteTableViewController, ToolbarUrlActionsProtocol {
    
    weak var toolbarUrlActionsDelegate: ToolbarUrlActionsDelegate?

    private lazy var emptyStateOverlayView: UIView = createEmptyStateOverlayView()

    private let spinner = UIActivityIndicatorView().then {
        $0.snp.makeConstraints { make in
            make.size.equalTo(24)
        }
        $0.hidesWhenStopped = true
        $0.isHidden = true
    }
    
    private var isLoading: Bool = false {
        didSet {
            if isLoading {
                self.view.addSubview(spinner)
                self.spinner.snp.makeConstraints {
                    $0.center.equalTo(view.snp.center)
                }
                self.spinner.startAnimating()
            } else {
                self.spinner.stopAnimating()
                self.spinner.removeFromSuperview()
            }
        }
    }

    private let historyAPI: BraveHistoryAPI
    
    var historyFRC: HistoryV2FetchResultsController?
    
    /// Certain bookmark actions are different in private browsing mode.
    let isPrivateBrowsing: Bool
    
    var isHistoryRefreshing = false
    
    private var searchHistoryTimer: Timer?
    private var isHistoryBeingSearched = false
    private let searchController = UISearchController(searchResultsController: nil)
    private var searchQuery = ""
    
    init(isPrivateBrowsing: Bool, historyAPI: BraveHistoryAPI) {
        self.isPrivateBrowsing = isPrivateBrowsing
        self.historyAPI = historyAPI
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
            #if swift(>=5.5)
            if #available(iOS 15.0, *) {
                $0.sectionHeaderTopPadding = 5
            }
            #endif
        }
            
        navigationItem.do {
            if !Preferences.Privacy.privateBrowsingOnly.value {
                $0.searchController = searchController
                $0.hidesSearchBarWhenScrolling = false
                $0.rightBarButtonItem =
                    UIBarButtonItem(image: #imageLiteral(resourceName: "playlist_delete_item").template, style: .done, target: self, action: #selector(performDeleteAll))
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
    
    private func createEmptyStateOverlayView() -> UIView {
        let overlayView = UIView().then {
            $0.backgroundColor = .secondaryBraveBackground
        }
        
        let logoImageView = UIImageView(image: #imageLiteral(resourceName: "emptyHistory").template).then {
            $0.tintColor = .braveLabel
        }
        
        let welcomeLabel = UILabel().then {
            $0.text = Preferences.Privacy.privateBrowsingOnly.value
                ? Strings.History.historyPrivateModeOnlyStateTitle
                : Strings.History.historyEmptyStateTitle
            $0.textAlignment = .center
            $0.font = DynamicFontHelper.defaultHelper.DeviceFontLight
            $0.textColor = .braveLabel
            $0.numberOfLines = 0
            $0.adjustsFontSizeToFitWidth = true
        }
                
        overlayView.addSubview(logoImageView)
        
        logoImageView.snp.makeConstraints { make in
            make.centerX.equalTo(overlayView)
            make.size.equalTo(60)
            // Sets proper top constraint for iPhone 6 in portait and for iPad.
            make.centerY.equalTo(overlayView).offset(-180).priority(100)
            // Sets proper top constraint for iPhone 4, 5 in portrait.
            make.top.greaterThanOrEqualTo(overlayView).offset(50)
        }
        
        overlayView.addSubview(welcomeLabel)
        
        welcomeLabel.snp.makeConstraints { make in
            make.centerX.equalTo(overlayView)
            make.top.equalTo(logoImageView.snp.bottom).offset(15)
            make.width.equalTo(170)
        }
        
        return overlayView
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
    
    @objc private func performDeleteAll() {
        let style: UIAlertController.Style = UIDevice.current.userInterfaceIdiom == .pad ? .alert : .actionSheet
        let alert = UIAlertController(
            title: Strings.History.historyClearAlertTitle, message: Strings.History.historyClearAlertDescription, preferredStyle: style)
        
        alert.addAction(UIAlertAction(title: Strings.History.historyClearActionTitle, style: .destructive, handler: { _ in
            DispatchQueue.main.async {
                self.historyAPI.removeAll {
                    self.refreshHistory()
                }
            }
        }))
        alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
        
        present(alert, animated: true, completion: nil)
    }
    
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
            cell.addGestureRecognizer(UILongPressGestureRecognizer(target: self, action: #selector(longPressedCell(_:))))
        }
        
        guard let historyItem = historyFRC?.object(at: indexPath) else { return }
        
        cell.do {
            $0.backgroundColor = UIColor.clear
            $0.setLines(historyItem.title, detailText: historyItem.url.absoluteString)
            
            $0.imageView?.contentMode = .scaleAspectFit
            $0.imageView?.image = FaviconFetcher.defaultFaviconImage
            $0.imageView?.layer.borderColor = BraveUX.faviconBorderColor.cgColor
            $0.imageView?.layer.borderWidth = BraveUX.faviconBorderWidth
            $0.imageView?.layer.cornerRadius = 6
            $0.imageView?.layer.cornerCurve = .continuous
            $0.imageView?.layer.masksToBounds = true
            
            let domain = Domain.getOrCreate(forUrl: historyItem.url,
                                            persistent: !PrivateBrowsingManager.shared.isPrivateBrowsing)
            
            if let url = domain.url?.asURL {
                cell.imageView?.loadFavicon(
                    for: url,
                    domain: domain,
                    fallbackMonogramCharacter: historyItem.title?.first,
                    shouldClearMonogramFavIcon: false,
                    cachedOnly: true)
            } else {
                cell.imageView?.clearMonogramFavicon()
                cell.imageView?.image = FaviconFetcher.defaultFaviconImage
            }
        }
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        guard let historyItem = historyFRC?.object(at: indexPath) else { return }
        
        if isHistoryBeingSearched {
            searchController.isActive = false
        }
            
        if let url = URL(string: historyItem.url.absoluteString) {
            dismiss(animated: true) {
                self.toolbarUrlActionsDelegate?.select(url: url, visitType: .typed)
            }
        }
        
        tableView.deselectRow(at: indexPath, animated: true)
    }
    
    @objc private func longPressedCell(_ gesture: UILongPressGestureRecognizer) {
        guard gesture.state == .began,
              let cell = gesture.view as? UITableViewCell,
              let indexPath = tableView.indexPath(for: cell),
              let urlString = historyFRC?.object(at: indexPath)?.url.absoluteString else {
            return
        }
        
        presentLongPressActions(gesture, urlString: urlString, isPrivateBrowsing: isPrivateBrowsing)
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
                
                if isHistoryBeingSearched {
                    reloadDataAndShowLoading(with: searchQuery)
                } else {
                    refreshHistory()
                }
            default:
                break
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
