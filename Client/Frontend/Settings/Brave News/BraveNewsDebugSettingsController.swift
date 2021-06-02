// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Static

extension FeedDataSource.Environment {
    fileprivate var name: String {
        switch self {
        case .dev: return "Dev"
        case .staging: return "Staging"
        case .production: return "Production"
        }
    }
}

extension FeedDataSource {
    fileprivate func description(of state: State) -> String {
        switch state {
        case .initial:
            return "—"
        case .loading:
            return "Loading"
        case .success:
            return "Success"
        case .failure(let error):
            return "Error: \(error.localizedDescription)"
        }
    }
    fileprivate func detailRows(for state: State) -> [Row] {
        switch state {
        case .initial:
            return []
        case .loading(let previousState):
            return [Row(text: "Previous State", detailText: description(of: previousState))]
        case .success(let cards):
            return [
                Row(text: "Sources", detailText: "\(sources.count)"),
                Row(text: "Cards Generated", detailText: "\(cards.count)")
            ]
        case .failure(let error):
            return [
                Row(text: "Error Details", detailText: error.localizedDescription, cellClass: MultilineSubtitleCell.self)
            ]
        }
    }
}

class BraveNewsDebugSettingsController: TableViewController {
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
        
        title = "Brave News QA Settings"
        
        reloadData()
        
        if navigationController?.viewControllers.first === self {
            navigationItem.rightBarButtonItem = .init(barButtonSystemItem: .done, target: self, action: #selector(tappedDone))
        }
    }
    
    func reloadData() {
        dataSource.sections = [
            .init(
                rows: [
                    Row(text: "Environment", detailText: feedDataSource.environment.name, selection: { [unowned self] in
                        let picker = TodayEnvironmentPicker(selectedEnvironment: feedDataSource.environment) { [unowned self] newEnvironment in
                            feedDataSource.environment = newEnvironment
                            self.reloadData()
                            self.navigationController?.popViewController(animated: true)
                        }
                        navigationController?.pushViewController(picker, animated: true)
                    }, accessory: .disclosureIndicator)
                ],
                footer: .title("Changing the environment will purge all cached resources immediately.")
            ),
            .init(
                rows: [
                    Row(text: "State", detailText: feedDataSource.description(of: feedDataSource.state)),
                ] + feedDataSource.detailRows(for: feedDataSource.state)
            ),
            .init(
                rows: [
                    Row(text: "Disable All Default Sources", selection: { [unowned self] in
                        let categories = Set(self.feedDataSource.sources.map(\.category))
                        for category in categories where !category.isEmpty {
                            self.feedDataSource.toggleCategory(category, enabled: false)
                        }
                    }, cellClass: ButtonCell.self)
                ]
            )
        ]
    }
    
    @objc private func tappedDone() {
        dismiss(animated: true)
    }
}

private class TodayEnvironmentPicker: TableViewController {
    let selectedEnvironment: FeedDataSource.Environment
    let environmentUpdated: (FeedDataSource.Environment) -> Void
    init(selectedEnvironment: FeedDataSource.Environment,
         environmentUpdated: @escaping (FeedDataSource.Environment) -> Void) {
        self.selectedEnvironment = selectedEnvironment
        self.environmentUpdated = environmentUpdated
        super.init(style: .insetGrouped)
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = "Environment"
        navigationItem.leftBarButtonItem = .init(barButtonSystemItem: .cancel, target: self, action: #selector(cancelPicker))
        
        dataSource.sections = [
            .init(rows: FeedDataSource.Environment.allCases.map { environment in
                Row(text: environment.name, selection: { [unowned self] in
                    self.environmentUpdated(environment)
                }, accessory: environment == selectedEnvironment ? .checkmark : .none)
            })
        ]
    }
    
    @objc private func cancelPicker() {
        navigationController?.popViewController(animated: true)
    }
}
