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
        super.init(style: .grouped)
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = "Brave Today" // FIXME: Localize
        
        if navigationController?.viewControllers.first === self {
            // Isolated presentation, add close button
            navigationItem.rightBarButtonItem = .init(barButtonSystemItem: .done, target: self, action: #selector(tappedDone))
        }
        
        dataSource.sections = [
            .init(
                rows: [
                    // FIXME: Localize
                    .boolRow(title: "Show Brave Today", option: Preferences.BraveToday.isEnabled)
                ]
            ),
            .init(
                rows: [
                    // FIXME: Localize
                    Row(text: "Choose Your Sources", selection: {
                        let controller = FeedSourceListViewController(dataSource: self.feedDataSource)
                        self.navigationController?.pushViewController(controller, animated: true)
                    }, accessory: .disclosureIndicator)
                ]
            )
        ]
        if !AppConstants.buildChannel.isPublic {
            // TODO: Add debug settings here
        }
    }
    
    @objc private func tappedDone() {
        dismiss(animated: true)
    }
}
