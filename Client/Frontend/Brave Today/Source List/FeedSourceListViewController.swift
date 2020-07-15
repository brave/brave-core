// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI

private class SourceTableViewCell: UITableViewCell, TableViewReusable {
    let enabledToggle = UISwitch()
    var enabledToggleValueChanged: ((Bool) -> Void)?
    
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        
        enabledToggle.addTarget(self, action: #selector(enabledToggleChanged), for: .valueChanged)
        
        selectionStyle = .none
//        backgroundColor = .clear
//        textLabel?.appearanceTextColor = .white
        textLabel?.numberOfLines = 0
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    override func prepareForReuse() {
        super.prepareForReuse()
        textLabel?.text = nil
        enabledToggle.isOn = false
    }
    
    @objc private func enabledToggleChanged() {
        enabledToggleValueChanged?(enabledToggle.isOn)
    }
}

class FeedSourceListViewController: UITableViewController {
    let dataSource: FeedDataSource
    var searchQuery: String?
    
    private var sources: [FeedItem.Source] {
        if let query = searchQuery, !query.isEmpty {
            return dataSource.sources.filter { $0.name.contains(query) }
        }
        return dataSource.sources
    }
    
    init(dataSource: FeedDataSource) {
        self.dataSource = dataSource
        super.init(style: .plain)
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    private let backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .dark)).then {
        $0.contentView.backgroundColor = UIColor(white: 1.0, alpha: 0.1)
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
//        if #available(iOS 13.0, *) {
//            tableView.appearanceOverrideUserInterfaceStyle = .dark
//        }
//        tableView.backgroundView = backgroundView
        
        tableView.register(SourceTableViewCell.self)
        tableView.estimatedRowHeight = UITableView.automaticDimension
        tableView.delegate = self
        tableView.dataSource = self
//        tableView.appearanceBackgroundColor = .clear
        
        title = "Choose Your Sources" // FIXME: Localize
        navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(tappedDone))
//        navigationController?.navigationBar.appearanceBarTintColor = nil
//        navigationController?.navigationBar.barStyle = .blackTranslucent
//        navigationController?.navigationBar.tintColor = .white
//        navigationController?.navigationBar.titleTextAttributes = [.foregroundColor: UIColor.white]
        
        let searchController = UISearchController(searchResultsController: nil)
        searchController.searchResultsUpdater = self
        searchController.searchBar.placeholder = "Search"
        searchController.obscuresBackgroundDuringPresentation = false
        navigationItem.hidesSearchBarWhenScrolling = true
        navigationItem.searchController = searchController
    }
    
    @objc private func tappedDone() {
        dismiss(animated: true)
    }
}

// MARK: - UISearchResultsUpdating
extension FeedSourceListViewController: UISearchResultsUpdating {
    func updateSearchResults(for searchController: UISearchController) {
        searchQuery = searchController.searchBar.text
        tableView.reloadData()
    }
}

// MARK: - UITableViewDelegate
//extension FeedSourceListViewController: UITableViewDelegate {
//}

// MARK: - UITableViewDataSource
extension FeedSourceListViewController {
    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return sources.count
    }
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(for: indexPath) as SourceTableViewCell
        guard let source = sources[safe: indexPath.row] else {
            assertionFailure()
            return cell
        }
        cell.selectionStyle = .none
        cell.textLabel?.text = source.name
        cell.accessoryView = cell.enabledToggle
        cell.enabledToggle.isOn = source.enabled
        cell.enabledToggleValueChanged = { isOn in
            self.dataSource.toggleSource(source, enabled: isOn)
        }
        return cell
    }
}
