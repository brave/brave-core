// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI
import BraveShared
import Shared

/// Displays a list of sources that may be optionally filtered down to a specific category
class FeedSourceListViewController: UITableViewController {
    /// The Brave Today feed data source
    let dataSource: FeedDataSource
    /// A category to filter the list of sources down to
    let category: String?
    
    private var searchQuery: String = ""
    private var sections: [[FeedItem.Source]] = []
    private var sectionIndexTitles: [String] = []
    private var customizedSources: [String: Bool] = [:]
    
    init(dataSource: FeedDataSource, category: String?) {
        self.dataSource = dataSource
        self.category = category
        super.init(style: .plain)
        reloadSections()
        if dataSource.sources.count > 1 {
            sectionIndexTitles = sections.compactMap { $0.first?.sectionIndexTitle }
        }
        tableView.tableFooterView = UIView()
        customizedSources = dataSource.customizedSources
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    private func reloadSections() {
        var list: [FeedItem.Source] = dataSource.sources
        if let category = category {
            list = list.filter { $0.category == category }
        }
        if !searchQuery.isEmpty {
            list = list
                .filter { $0.name.lowercased().contains(searchQuery.lowercased()) }
        }
        var updatedSections: [[FeedItem.Source]] = []
        let otherSections = Dictionary(grouping: list, by: { $0.sectionIndexTitle ?? "" })
            .filter { !$0.key.isEmpty }
            .sorted(by: { $0.key < $1.key })
            .reduce(into: [[FeedItem.Source]]()) { (sections, value) in
                sections.append(value.value)
        }
        if !otherSections.isEmpty {
            updatedSections.append(contentsOf: otherSections)
        }
        sections = updatedSections
        if isViewLoaded {
            tableView.reloadData()
        }
    }
    
    private let backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .dark)).then {
        $0.contentView.backgroundColor = UIColor(white: 1.0, alpha: 0.1)
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        tableView.register(SourceTableViewCell.self)
        tableView.estimatedRowHeight = UITableView.automaticDimension
        tableView.delegate = self
        tableView.dataSource = self
        tableView.sectionIndexColor = BraveUX.braveOrange
        
        title = category ?? Strings.BraveToday.allSources
        if category != nil {
            // TODO(iOS14): Replace with context menu primary action
            navigationItem.rightBarButtonItem = .init(
                image: UIImage(imageLiteralResourceName: "menu_more"),
                style: .plain,
                target: self,
                action: #selector(tappedMenu(_:))
            )
        }
        
        let searchController = UISearchController(searchResultsController: nil)
        searchController.searchResultsUpdater = self
        searchController.searchBar.placeholder = Strings.BraveToday.sourceSearchPlaceholder
        searchController.obscuresBackgroundDuringPresentation = false
        navigationItem.hidesSearchBarWhenScrolling = true
        navigationItem.searchController = searchController
    }
    
    @objc private func tappedMenu(_ sender: UIBarButtonItem) {
        let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
        alert.popoverPresentationController?.barButtonItem = sender
        alert.addAction(UIAlertAction(title: Strings.BraveToday.enableAll, style: .default, handler: { [weak self] _ in
            self?.toggleCategory(enabled: true)
        }))
        alert.addAction(UIAlertAction(title: Strings.BraveToday.disableAll, style: .destructive, handler: { [weak self] _ in
            self?.toggleCategory(enabled: false)
        }))
        alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel))
        present(alert, animated: true)
    }
    
    private func toggleCategory(enabled: Bool) {
        guard let category = category else { return }
        // Update DB
        dataSource.toggleCategory(category, enabled: enabled)
        // Update local state
        for section in sections {
            for source in section {
                customizedSources[source.id] = enabled
            }
        }
        for cell in tableView.visibleCells {
            if let cell = cell as? SourceTableViewCell {
                cell.enabledToggle.setOn(enabled, animated: true)
            }
        }
    }
}

// MARK: - UISearchResultsUpdating
extension FeedSourceListViewController: UISearchResultsUpdating {
    func updateSearchResults(for searchController: UISearchController) {
        searchQuery = searchController.searchBar.text ?? ""
        reloadSections()
        tableView.separatorStyle = sections.isEmpty ? .none : .singleLine
    }
}

// MARK: - UITableViewDataSource
extension FeedSourceListViewController {
    override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        guard let source = sections[safe: section]?[safe: 0] else {
            return nil
        }
        return source.sectionIndexTitle
    }
    
    override func sectionIndexTitles(for tableView: UITableView) -> [String]? {
        if !searchQuery.isEmpty {
            return nil
        }
        return sectionIndexTitles
    }
    
    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return sections[section].count
    }
    
    override func numberOfSections(in tableView: UITableView) -> Int {
        return sections.count
    }
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(for: indexPath) as SourceTableViewCell
        guard let source = sections[safe: indexPath.section]?[safe: indexPath.row] else {
            assertionFailure()
            return cell
        }
        cell.selectionStyle = .none
        cell.textLabel?.text = source.name
        cell.accessoryView = cell.enabledToggle
        cell.enabledToggle.isOn = customizedSources[source.id] ?? source.isDefault
        cell.enabledToggleValueChanged = { [weak self] isOn in
            guard let self = self else { return }
            // Update data source + DB
            self.dataSource.toggleSource(source, enabled: isOn)
            // Update local state
            self.customizedSources[source.id] = isOn
        }
        return cell
    }
}

extension FeedItem.Source {
    fileprivate var sectionIndexTitle: String? {
        if let firstCharacter = name.first?.uppercased() {
            if Int(firstCharacter) != nil {
                return "#"
            }
            return firstCharacter
        }
        return nil
    }
}

private class SourceTableViewCell: UITableViewCell, TableViewReusable {
    let enabledToggle = UISwitch()
    var enabledToggleValueChanged: ((Bool) -> Void)?
    
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        
        enabledToggle.addTarget(self, action: #selector(enabledToggleChanged), for: .valueChanged)
        
        selectionStyle = .none
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
