// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import Shared

class BraveTodayAddSourceResultsViewController: UITableViewController {
    
    let feedDataSource: FeedDataSource
    let searchedURL: URL
    private let secureLocations: [RSSFeedLocation]
    private let insecureLocations: [RSSFeedLocation]
    var sourcesAdded: ((Set<RSSFeedLocation>) -> Void)?
    
    private var selectedLocations: Set<RSSFeedLocation>
    
    init(dataSource: FeedDataSource,
         searchedURL: URL,
         rssFeedLocations: [RSSFeedLocation],
         sourcesAdded: ((Set<RSSFeedLocation>) -> Void)?
    ) {
        self.feedDataSource = dataSource
        self.searchedURL = searchedURL
        let locations = Set(rssFeedLocations)
        self.secureLocations = locations.filter { $0.url.scheme == "https" }
        self.insecureLocations = Array(locations.subtracting(self.secureLocations))
        self.selectedLocations = locations
        self.sourcesAdded = sourcesAdded
        
        if #available(iOS 13.0, *) {
            super.init(style: .insetGrouped)
        } else {
            super.init(style: .grouped)
        }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    private lazy var doneButton = UIBarButtonItem(
        title: Strings.BraveToday.addSourceAddButtonTitle,
        style: .done,
        target: self,
        action: #selector(tappedAdd)
    )
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = searchedURL.baseDomain
        
        navigationItem.largeTitleDisplayMode = .never
        navigationItem.rightBarButtonItem = doneButton
        
        tableView.register(FeedLocationCell.self)
        
        if navigationController?.viewControllers.first === self {
            // Presented via share screen or isolated
            navigationItem.leftBarButtonItem = .init(barButtonSystemItem: .cancel, target: self, action: #selector(tappedCancel))
        }
    }
    
    private func showMaximumReachedAlert() {
        let alert = UIAlertController(
            title: Strings.BraveToday.rssFeedLimitExceededAlertTitle,
            message: Strings.BraveToday.rssFeedLimitExceededAlertMessage,
            preferredStyle: .alert
        )
        alert.addAction(.init(title: Strings.OKString, style: .default))
        present(alert, animated: true)
    }
    
    @objc private func tappedAdd() {
        let numberOfAddedFeeds = feedDataSource.rssFeedLocations.count
        if numberOfAddedFeeds + selectedLocations.count > FeedDataSource.maximumNumberOfRSSFeeds {
            showMaximumReachedAlert()
            return
        }
        // Add selected sources to feed
        for location in selectedLocations {
            feedDataSource.addRSSFeedLocation(location)
        }
        sourcesAdded?(selectedLocations)
        dismiss(animated: true)
    }
    
    @objc private func tappedCancel() {
        dismiss(animated: true)
    }
    
    // MARK: - UITableViewDelegate
    
    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        let locations = indexPath.section == 0 ? secureLocations : insecureLocations
        if let location = locations[safe: indexPath.row],
           let cell = tableView.cellForRow(at: indexPath) as? FeedLocationCell {
            if selectedLocations.remove(location) == nil {
                selectedLocations.insert(location)
            }
            cell.accessoryType = selectedLocations.contains(location) ? .checkmark : .none
            doneButton.isEnabled = !selectedLocations.isEmpty
        }
        tableView.deselectRow(at: indexPath, animated: true)
    }
    
    // MARK: - UITableViewDataSource
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let locations = indexPath.section == 0 ? secureLocations : insecureLocations
        guard let location = locations[safe: indexPath.row] else {
            assertionFailure()
            return UITableViewCell()
        }
        let cell = tableView.dequeueReusableCell(for: indexPath) as FeedLocationCell
        cell.imageView?.image = indexPath.section == 0 ? #imageLiteral(resourceName: "lock_verified").template : #imageLiteral(resourceName: "insecure-site-icon")
        cell.imageView?.tintColor = .braveLabel
        cell.textLabel?.text = location.title
        cell.detailTextLabel?.text = location.url.absoluteString
        cell.accessoryType = selectedLocations.contains(location) ? .checkmark : .none
        return cell
    }
    
    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        section == 0 ? secureLocations.count : insecureLocations.count
    }
    
    override func numberOfSections(in tableView: UITableView) -> Int {
        insecureLocations.isEmpty ? 1 : 2
    }
    
    override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        if section == 1 {
            return Strings.BraveToday.insecureSourcesHeader
        }
        return nil
    }
    
    override func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
        if section == tableView.numberOfSections - 1 {
            let feedCount = feedDataSource.rssFeedLocations.count
            return String.localizedStringWithFormat(Strings.BraveToday.rssFeedLimitRemainingFooter, feedCount)
        }
        return nil
    }
}

private class FeedLocationCell: UITableViewCell, TableViewReusable {
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: .subtitle, reuseIdentifier: reuseIdentifier)
    }
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
}
