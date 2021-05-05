// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Static
import Shared
import BraveShared
import Data

/// Displays relevant Brave Today settings such as toggling the feature on/off, and selecting sources
///
/// This controller may be presented in an isolated environment outside of the main settings pages from the
/// Brave Today header on the NTP
class BraveTodaySettingsViewController: TableViewController {
    
    private let feedDataSource: FeedDataSource
    
    init(dataSource: FeedDataSource) {
        feedDataSource = dataSource
        super.init(style: .insetGrouped)
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
        
        reloadSections()
    }
    
    private func reloadSections() {
        // If a user hasn't opted in, they must do so before using Brave Today's features
        if !Preferences.BraveToday.userOptedIn.value {
            dataSource.sections = [
                .init(
                    rows: [
                        .init(
                            text: Strings.BraveToday.braveToday,
                            detailText: Strings.BraveToday.introCardBody,
                            cellClass: MultilineSubtitleCell.self
                        ),
                        .init(
                            text: Strings.BraveToday.turnOnBraveToday,
                            selection: { [unowned self] in
                                Preferences.BraveToday.isShowingOptIn.value = false
                                Preferences.BraveToday.userOptedIn.value = true
                                Preferences.BraveToday.isEnabled.value = true
                                if self.feedDataSource.shouldLoadContent {
                                    self.feedDataSource.load()
                                }
                                self.reloadSections()
                            },
                            cellClass: CenteredButtonCell.self
                        )
                    ]
                )
            ]
            return
        }
        dataSource.sections = [
            .init(
                rows: [
                    .boolRow(
                        title: Strings.BraveToday.isEnabledToggleLabel,
                        option: Preferences.BraveToday.isEnabled
                    )
                ]
            ),
            .init(
                header: .title(Strings.BraveToday.yourSources),
                rows: feedDataSource.rssFeedLocations.map { location in
                    let enabled = self.feedDataSource.isRSSFeedEnabled(location)
                    return Row(
                        text: location.title,
                        detailText: location.url.absoluteString,
                        accessory: .switchToggle(value: enabled, { [unowned self] newValue in
                            self.feedDataSource.toggleRSSFeedEnabled(location, enabled: newValue)
                        }),
                        cellClass: SubtitleCell.self,
                        editActions: [.init(title: Strings.BraveToday.deleteUserSourceTitle, style: .destructive, selection: { [unowned self] indexPath in
                            guard let location = feedDataSource.rssFeedLocations[safe: indexPath.row] else { return }
                            self.feedDataSource.removeRSSFeed(location)
                            self.reloadSections()
                        })]
                    )
                } + [
                    Row(text: Strings.BraveToday.addSource, selection: { [unowned self] in
                        let controller = BraveTodayAddSourceViewController(dataSource: self.feedDataSource)
                        controller.sourcesAdded = { [weak self] _ in
                            self?.reloadSections()
                        }
                        let container = UINavigationController(rootViewController: controller)
                        let idiom = UIDevice.current.userInterfaceIdiom
                        if #available(iOS 13.0, *) {
                            container.modalPresentationStyle = idiom == .phone ? .pageSheet : .formSheet
                        } else {
                            container.modalPresentationStyle = idiom == .phone ? .fullScreen : .formSheet
                        }
                        self.present(container, animated: true)
                    }, image: nil, accessory: .disclosureIndicator)
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
                .filter { !$0.isEmpty }
                .sorted()
                .map(row(for:))
        )
        return rows
    }
    
    @objc private func tappedDone() {
        dismiss(animated: true)
    }
}
