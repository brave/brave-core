// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Static
import Shared
import BraveShared

private let log = Logger.browserLogger

class RetentionPreferencesDebugMenuViewController: TableViewController {
    
    init() {
        super.init(style: .insetGrouped)
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = "Retention Preferences"
        
        dataSource.sections = [
            startOnboardingSection,
            debugFlags,
            retentionPreferenceFlags        ]
    }
    
    private func presentDebugFlagAlert() {
        let alert = UIAlertController(
            title: "Value can't be changed!",
            message: "This is debug flag value cant be changed.",
            preferredStyle: .alert)
        alert.addAction(.init(title: "OK", style: .default, handler: nil))

        present(alert, animated: true, completion: nil)
    }
    
    // MARK: - Sections
    
    private lazy var startOnboardingSection: Static.Section = {
        var section = Static.Section(
            rows: [
                .init(text: "Start Onboarding", selection: { [unowned self] in
                    let onboardingController = WelcomeViewController(profile: nil,
                                                                     rewards: nil)
                    onboardingController.modalPresentationStyle = .fullScreen
                    onboardingController.onAdsWebsiteSelected = { [weak self] url in
                        self?.dismiss(animated: true)
                    }
                    onboardingController.onSkipSelected = { [weak self] in
                        self?.dismiss(animated: true)
                    }
                    
                    present(onboardingController, animated: false)
                }, cellClass: MultilineButtonCell.self)
            ]
        )
        
        return section
    }()
    
    private lazy var debugFlags: Section = {
        var shields = Section(
            header: .title("Debug Flags"),
            rows: [
                .boolRow(
                    title: "Skip Onboarding Intro",
                    detailText: "Flag for hide/show entire onboarding sequence.",
                    toggleValue: Preferences.DebugFlag.skipOnboardingIntro ?? false,
                    valueChange: { [unowned self] _ in
                        self.presentDebugFlagAlert()
                    },
                    cellReuseId: "SkipOnboardingCell"),
                .boolRow(
                    title: "Skip Education Pop-ups",
                    detailText: "Flag for hide/show education pop-ups. Includes onboarding ad block notifications",
                    toggleValue: Preferences.DebugFlag.skipEduPopups ?? false,
                    valueChange: { [unowned self] _ in
                        self.presentDebugFlagAlert()
                    },
                    cellReuseId: "SkipEduCell"),
                .boolRow(
                    title: "Skip NTP Callouts",
                    detailText: "Flag for hide/show full screen callouts. Includes Default Browser, Rewards, Sync",
                    toggleValue: Preferences.DebugFlag.skipNTPCallouts ?? false,
                    valueChange: { [unowned self] _ in
                        self.presentDebugFlagAlert()
                    },
                    cellReuseId: "SkipNTPCell")
            ],
            footer: .title("These are the debug flags that enables entire features and set to false for Debug scheme in order to provide faster development.")
        )
        return shields
    }()
    
    private lazy var retentionPreferenceFlags: Section = {
        var shields = Section(
            header: .title("Retention Preferences"),
            rows: [
                .boolRow(
                    title: "Retention User",
                    detailText: "Flag showing if the user installed the application after new onboarding is added.",
                    toggleValue: Preferences.General.isNewRetentionUser.value ?? false,
                    valueChange: {
                        if $0 {
                            let status = $0
                            Preferences.General.isNewRetentionUser.value = status
                        }
                    },
                    cellReuseId: "RetentionUserCell"),
                .boolRow(
                    title: "NTP Education Shown",
                    detailText: "Flag tracking NTP Education should be loaded after onboarding of user.",
                    toggleValue: Preferences.FullScreenCallout.ntpCalloutCompleted.value,
                    valueChange: {
                        if $0 {
                            let status = $0
                            Preferences.FullScreenCallout.ntpCalloutCompleted.value = status
                        }
                    },
                    cellReuseId: "NTPEducationCell"),
                .boolRow(
                    title: "VPN Callout Shown",
                    detailText: "Flag determining if VPN callout is shown to user.",
                    toggleValue: Preferences.FullScreenCallout.vpnCalloutCompleted.value,
                    valueChange: {
                        if $0 {
                            let status = $0
                            Preferences.FullScreenCallout.vpnCalloutCompleted.value = status
                        }
                    },
                    cellReuseId: "VPNCalloutCell"),
                .boolRow(
                    title: "Sync Callout Shown",
                    detailText: "Flag determining if Sync callout is shown to user.",
                    toggleValue: Preferences.FullScreenCallout.syncCalloutCompleted.value,
                    valueChange: {
                        if $0 {
                            let status = $0
                            Preferences.FullScreenCallout.syncCalloutCompleted.value = status
                        }
                    },
                    cellReuseId: "SyncCalloutCell"),
                .boolRow(
                    title: "Rewards Callout Shown",
                    detailText: "Flag determining if Rewards callout is shown to user.",
                    toggleValue: Preferences.FullScreenCallout.vpnCalloutCompleted.value,
                    valueChange: {
                        if $0 {
                            let status = $0
                            Preferences.FullScreenCallout.rewardsCalloutCompleted.value = status
                        }
                    },
                    cellReuseId: "RewardsCalloutCell"),
                .boolRow(
                    title: "Default Browser Callout Shown",
                    detailText: "Flag determining if DefaultBrowser callout is shown to user.",
                    toggleValue: Preferences.DefaultBrowserIntro.completed.value,
                    valueChange: {
                        if $0 {
                            let status = $0
                            Preferences.DefaultBrowserIntro.completed.value = status
                        }
                    },
                    cellReuseId: "DefaultBrowserCalloutCell")
                ],
                footer: .title("These are the preferences that stored in preferences for determining the If certain elements are shown to user.")
        )
        return shields
    }()
}
