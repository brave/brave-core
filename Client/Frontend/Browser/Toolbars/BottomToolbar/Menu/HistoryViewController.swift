/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import BraveShared
import Storage
import Data
import CoreData

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
    
    var historyFRC: HistoryV2FetchResultsController?
    
    /// Certain bookmark actions are different in private browsing mode.
    let isPrivateBrowsing: Bool
    
    var isHistoryRefreshing = false
    
    init(isPrivateBrowsing: Bool) {
        self.isPrivateBrowsing = isPrivateBrowsing
        super.init(nibName: nil, bundle: nil)
        
        historyFRC = Historyv2.frc()
        historyFRC?.delegate = self
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        tableView.accessibilityIdentifier = "History List"
        title = Strings.historyScreenTitle
                
        if !Preferences.Privacy.privateBrowsingOnly.value {
            navigationItem.rightBarButtonItem =
                UIBarButtonItem(image: #imageLiteral(resourceName: "playlist_delete_item").template, style: .done, target: self, action: #selector(performDeleteAll))
        }
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        refreshHistory()
    }
    
    private func refreshHistory() {
        if Preferences.Privacy.privateBrowsingOnly.value {
            showEmptyPanelState()
        } else {
            if !isHistoryRefreshing {
                view.addSubview(spinner)
                spinner.snp.makeConstraints {
                    $0.center.equalTo(view.snp.center)
                }
                spinner.startAnimating()
                isHistoryRefreshing = true

                Historyv2.waitForHistoryServiceLoaded { [weak self] in
                    guard let self = self else { return }
                    
                    self.reloadData() {
                        self.isHistoryRefreshing = false
                        self.spinner.stopAnimating()
                        self.spinner.removeFromSuperview()
                    }
                }
            }
        }
    }
    
    private func reloadData(_ completion: @escaping () -> Void) {
        // Recreate the frc if it was previously removed
        if historyFRC == nil {
            historyFRC = Historyv2.frc()
            historyFRC?.delegate = self
        }
        
        historyFRC?.performFetch { [weak self] in
            guard let self = self else { return }
            
            self.tableView.reloadData()
            self.updateEmptyPanelState()
            
            completion()
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
    
    @objc private func performDeleteAll() {
        let style: UIAlertController.Style = UIDevice.current.userInterfaceIdiom == .pad ? .alert : .actionSheet
        let alert = UIAlertController(
            title: Strings.History.historyClearAlertTitle, message: Strings.History.historyClearAlertDescription, preferredStyle: style)
        
        alert.addAction(UIAlertAction(title: Strings.History.historyClearActionTitle, style: .destructive, handler: { _ in
            DispatchQueue.main.async {
                Historyv2.deleteAll { [weak self] in
                    self?.refreshHistory()
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
        guard let cell = cell as? TwoLineTableViewCell else { return }
        
        if !tableView.isEditing {
            cell.gestureRecognizers?.forEach { cell.removeGestureRecognizer($0) }
            cell.addGestureRecognizer(UILongPressGestureRecognizer(target: self, action: #selector(longPressedCell(_:))))
        }
        
        guard let historyItem = historyFRC?.object(at: indexPath) else { return }
        
        cell.do {
            $0.backgroundColor = UIColor.clear
            $0.setLines(historyItem.title, detailText: historyItem.url)
            
            $0.imageView?.contentMode = .scaleAspectFit
            $0.imageView?.image = FaviconFetcher.defaultFaviconImage
            $0.imageView?.layer.borderColor = BraveUX.faviconBorderColor.cgColor
            $0.imageView?.layer.borderWidth = BraveUX.faviconBorderWidth
            $0.imageView?.layer.cornerRadius = 6
            $0.imageView?.layer.cornerCurve = .continuous
            $0.imageView?.layer.masksToBounds = true
            
            if let url = historyItem.domain?.asURL {
                cell.imageView?.loadFavicon(for: url)
            } else {
                cell.imageView?.clearMonogramFavicon()
                cell.imageView?.image = FaviconFetcher.defaultFaviconImage
            }
        }
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        guard let historyItem = historyFRC?.object(at: indexPath) else { return }
        
        if let historyURL = historyItem.url, let url = URL(string: historyURL) {
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
              let urlString = historyFRC?.object(at: indexPath)?.url else {
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
                historyItem.delete()
                
                refreshHistory()
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
