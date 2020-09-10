// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Static
import Shared
import BraveShared

/// Displays relevant Brave Today settings such as toggling the feature on/off, and selecting sources
///
/// This controller may be presented in an isolated environment outside of the main settings pages from the
/// Brave Today header on the NTP
class BraveTodaySettingsViewController: TableViewController {
    
    private let feedDataSource: FeedDataSource
    
    init(dataSource: FeedDataSource) {
        feedDataSource = dataSource
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
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = Strings.BraveToday.braveToday
        
        if navigationController?.viewControllers.first === self {
            // Isolated presentation, add close button
            navigationItem.rightBarButtonItem = .init(barButtonSystemItem: .done, target: self, action: #selector(tappedDone))
        }
        
        dataSource.sections = [
            .init(
                rows: [
                    .boolRow(title: Strings.BraveToday.isEnabledToggleLabel, option: Preferences.BraveToday.isEnabled)
                ]
            )
        ]
        
        if !feedDataSource.sources.isEmpty {
            dataSource.sections.append(
                .init(
                    header: .title(Strings.BraveToday.settingsSourceHeaderTitle),
                    rows: [
                        Row(text: Strings.BraveToday.allSources, selection: { [unowned self] in
                            let controller = FeedSourceListViewController(dataSource: self.feedDataSource, category: nil)
                            self.navigationController?.pushViewController(controller, animated: true)
                        }, accessory: .disclosureIndicator)
                    ] + categoryRows
                )
            )
            dataSource.sections.append(
                .init(rows: [
                    Row(text: Strings.BraveToday.resetSourceSettingsButtonTitle, selection: { [unowned self] in
                        self.feedDataSource.resetSourcesToDefault()
                    }, cellClass: ButtonCell.self)
                ])
            )
        }
        
        if !AppConstants.buildChannel.isPublic {
            // TODO: Add debug settings here
        }
    }
    
    private var categoryRows: [Row] {
        var rows: [Row] = []
        var categories = Set(feedDataSource.sources.map(\.category))
        let topNewsCategory = FeedDataSource.topNewsCategory
        func row(for category: String) -> Row {
            Row(text: category, selection: {
                let controller = FeedSourceListViewController(dataSource: self.feedDataSource, category: category)
                self.navigationController?.pushViewController(controller, animated: true)
            }, accessory: .disclosureIndicator)
        }
        if categories.contains(topNewsCategory) {
            categories.remove(topNewsCategory)
            rows.append(row(for: topNewsCategory))
        }
        rows.append(contentsOf:
            categories
                .sorted()
                .map(row(for:))
        )
        return rows
    }
    
    @objc private func tappedDone() {
        dismiss(animated: true)
    }
}
