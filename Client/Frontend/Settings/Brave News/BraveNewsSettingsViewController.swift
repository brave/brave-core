// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Static
import Shared
import BraveShared
import Data
import BraveCore

/// Displays relevant Brave News settings such as toggling the feature on/off, and selecting sources
///
/// This controller may be presented in an isolated environment outside of the main settings pages from the
/// Brave News header on the NTP
class BraveNewsSettingsViewController: TableViewController {
    
    private let feedDataSource: FeedDataSource
    private let rewards: BraveRewards?
    
    init(dataSource: FeedDataSource, rewards: BraveRewards?) {
        feedDataSource = dataSource
        self.rewards = rewards
        super.init(style: .insetGrouped)
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = Strings.BraveNews.braveNews
        
        if navigationController?.viewControllers.first === self {
            // Isolated presentation, add close button
            navigationItem.rightBarButtonItem = .init(barButtonSystemItem: .done, target: self, action: #selector(tappedDone))
        }
        
        reloadSections()
    }
    
    private func reloadSections() {
        // If a user hasn't opted in, they must do so before using Brave News's features
        if !Preferences.BraveNews.userOptedIn.value {
            dataSource.sections = [
                .init(
                    rows: [
                        .init(
                            text: Strings.BraveNews.braveNews,
                            detailText: Strings.BraveNews.introCardBody,
                            cellClass: MultilineSubtitleCell.self
                        ),
                        .init(
                            text: Strings.BraveNews.turnOnBraveNews,
                            selection: { [unowned self] in
                                Preferences.BraveNews.isShowingOptIn.value = false
                                Preferences.BraveNews.userOptedIn.value = true
                                Preferences.BraveNews.isEnabled.value = true
                                if self.feedDataSource.shouldLoadContent {
                                    if let rewards = rewards {
                                        rewards.ads.initialize { [weak self] _ in
                                            self?.feedDataSource.load()
                                        }
                                    } else {
                                        feedDataSource.load()
                                    }
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
                        title: Strings.BraveNews.isEnabledToggleLabel,
                        option: Preferences.BraveNews.isEnabled,
                        onValueChange: { [unowned self] value in
                            Preferences.BraveNews.isEnabled.value = value
                            if value {
                                self.rewards?.ads.initialize { _ in }
                            }
                        }
                    )
                ]
            ),
            .init(
                header: .title(Strings.BraveNews.yourSources),
                rows: feedDataSource.rssFeedLocations.map { location in
                    let enabled = self.feedDataSource.isRSSFeedEnabled(location)
                    return Row(
                        text: location.title,
                        detailText: location.url.absoluteString,
                        accessory: .switchToggle(value: enabled, { [unowned self] newValue in
                            self.feedDataSource.toggleRSSFeedEnabled(location, enabled: newValue)
                        }),
                        cellClass: SubtitleCell.self,
                        editActions: [.init(title: Strings.BraveNews.deleteUserSourceTitle, style: .destructive, selection: { [unowned self] indexPath in
                            guard let location = feedDataSource.rssFeedLocations[safe: indexPath.row] else { return }
                            self.feedDataSource.removeRSSFeed(location)
                            self.reloadSections()
                        })]
                    )
                } + [
                    Row(text: Strings.BraveNews.addSource, selection: { [unowned self] in
                        let controller = BraveNewsAddSourceViewController(dataSource: self.feedDataSource)
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
                    header: .title(Strings.BraveNews.settingsSourceHeaderTitle),
                    rows: [
                        Row(text: Strings.BraveNews.allSources, selection: { [unowned self] in
                            let controller = FeedSourceListViewController(dataSource: self.feedDataSource, category: nil)
                            self.navigationController?.pushViewController(controller, animated: true)
                        }, accessory: .disclosureIndicator)
                    ] + categoryRows
                )
            )
            dataSource.sections.append(
                .init(rows: [
                    Row(text: Strings.BraveNews.resetSourceSettingsButtonTitle, selection: { [unowned self] in
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
