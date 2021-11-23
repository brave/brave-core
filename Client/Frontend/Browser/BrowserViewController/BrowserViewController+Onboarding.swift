// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveShared
import BraveUI
import Shared
import BraveCore

// MARK: - Onboarding

extension BrowserViewController {

    func presentOnboardingIntro() {        
        if Preferences.DebugFlag.skipOnboardingIntro == true { return }
        
        // 1. Existing user.
        // 2. User already completed onboarding.
        if Preferences.General.basicOnboardingCompleted.value == OnboardingState.completed.rawValue {
            return
        }
        
        // 1. User is brand new
        // 2. User hasn't completed onboarding
        if Preferences.General.basicOnboardingCompleted.value != OnboardingState.completed.rawValue,
           Preferences.General.isNewRetentionUser.value == true {
            let onboardingController = WelcomeViewController(profile: profile,
                                                             rewards: rewards)
            onboardingController.modalPresentationStyle = .fullScreen
            onboardingController.onAdsWebsiteSelected = { [weak self] url in
                guard let self = self else { return }
                
                if let url = url {
                    let isPrivate = PrivateBrowsingManager.shared.isPrivateBrowsing
                    self.topToolbar.leaveOverlayMode()
                    let tab = self.tabManager.addTab(PrivilegedRequest(url: url) as URLRequest,
                                                     afterTab: self.tabManager.selectedTab,
                                                     isPrivate: isPrivate)
                    self.tabManager.selectTab(tab)
                } else {
                    DispatchQueue.main.asyncAfter(deadline: .now() + 0.25) {
                        self.topToolbar.tabLocationViewDidTapLocation(self.topToolbar.locationView)
                    }
                }
            }
            
            present(onboardingController, animated: false)
            isOnboardingOrFullScreenCalloutPresented = true
            shouldShowNTPEducation = true
        }
    }
    
    func presentNTPStatsOnboarding() {
        guard let ntpController = tabManager.selectedTab?.newTabPageViewController,
            let statsFrame = ntpController.ntpStatsOnboardingFrame else {
            return
        }
        
        // Project the statsFrame to the current frame
        let frame = view.convert(statsFrame, from: ntpController.view).insetBy(dx: 15.0, dy: 15.0)
        
        // Create a border view
        let borderView = UIView().then {
            let borderLayer = CAShapeLayer().then {
                let frame = frame.with { $0.origin = .zero }
                $0.strokeColor = UIColor.white.cgColor
                $0.fillColor = UIColor.clear.cgColor
                $0.lineWidth = 2.0
                $0.strokeEnd = 1.0
                $0.path = UIBezierPath(roundedRect: frame, cornerRadius: 12.0).cgPath
            }
            $0.layer.addSublayer(borderLayer)
        }
        
        view.addSubview(borderView)
        borderView.frame = frame
        
        // Present the popover
        let controller = WelcomeNTPOnboardingController()
        controller.setText(details: Strings.Onboarding.ntpOnboardingPopOverTrackerDescription)
        
        let popover = PopoverController(contentController: controller)
        popover.arrowDistance = 10.0
        popover.present(from: borderView, on: self)
        
        // Mask the shadow
        let maskFrame = view.convert(frame, to: popover.backgroundOverlayView)
        let maskShape = CAShapeLayer().then {
            $0.fillRule = .evenOdd
            $0.fillColor = UIColor.white.cgColor
            $0.strokeColor = UIColor.clear.cgColor
        }
        
        popover.backgroundOverlayView.layer.mask = maskShape
        popover.popoverDidDismiss = { [weak self] _ in
            maskShape.removeFromSuperlayer()
            borderView.removeFromSuperview()
            Preferences.FullScreenCallout.ntpCalloutCompleted.value = true
            self?.presentNTPMenuOnboarding()
        }
        
        DispatchQueue.main.async {
            maskShape.path = {
                let path = CGMutablePath()
                path.addRect(popover.backgroundOverlayView.bounds)
                path.addRoundedRect(in: maskFrame,
                                    cornerWidth: 12.0,
                                    cornerHeight: 12.0)
                return path
            }()
        }
    }
    
    func presentNTPMenuOnboarding() {
        guard let menuButton = UIDevice.isIpad ? topToolbar.menuButton : toolbar?.menuButton else { return }
        let controller = WelcomeNTPOnboardingController()
        controller.setText(
            title: Strings.Onboarding.ntpOnboardingPopoverDoneTitle,
            details: Strings.Onboarding.ntpOnboardingPopoverDoneDescription)
        
        let popover = PopoverController(contentController: controller)
        popover.arrowDistance = 7.0
        popover.present(from: menuButton, on: self)
        
        if let icon = menuButton.imageView?.image {
            let maskedView = controller.maskedPointerView(icon: icon,
                                                          tint: menuButton.imageView?.tintColor)
            popover.view.insertSubview(maskedView, aboveSubview: popover.backgroundOverlayView)
            maskedView.frame = CGRect(width: 45.0, height: 45.0)
            maskedView.center = view.convert(menuButton.center, from: menuButton.superview)
            maskedView.layer.cornerRadius = max(maskedView.bounds.width, maskedView.bounds.height) / 2.0
            
            popover.popoverDidDismiss = { _ in
                maskedView.removeFromSuperview()
            }
        }
    }
    
    func notifyTrackersBlocked(domain: String, trackers: [String: [String]]) {
        let controller = WelcomeBraveBlockedAdsController().then {
            var trackers = trackers
            let first = trackers.popFirst()
            let tracker = first?.key
            let trackerCount = ((first?.value.count ?? 0) - 1) + trackers.reduce(0, { res, values in
                res + values.value.count
            })
            
            $0.setData(domain: domain, trackerBlocked: tracker ?? "", trackerCount: trackerCount)
        }
        
        let popover = PopoverController(contentController: controller)
        popover.present(from: topToolbar.locationView.shieldsButton, on: self)
        
        let pulseAnimation = RadialPulsingAnimation(ringCount: 3)
        pulseAnimation.present(icon: topToolbar.locationView.shieldsButton.imageView?.image,
                               from: topToolbar.locationView.shieldsButton,
                               on: popover,
                               browser: self)
        popover.popoverDidDismiss = { _ in
            pulseAnimation.removeFromSuperview()
        }
    }
    
    /// New Tab Page Education screen should load after onboarding is finished and user is on locale JP
    /// - Returns: A tuple which shows NTP Edication is enabled and URL to be loaed
    fileprivate func showNTPEducation() -> (isEnabled: Bool, url: URL?) {
        guard let url = BraveUX.ntpTutorialPageURL else {
            return (false, nil)
        }

        return (Locale.current.regionCode == "JP", url)
    }
}

// MARK: OnboardingControllerDelegate

extension BrowserViewController {
    private func presentEducationNTPIfNeeded() {
        // NTP Education Load after onboarding screen
        if shouldShowNTPEducation,
           showNTPEducation().isEnabled,
           let url = showNTPEducation().url {
            tabManager.selectedTab?.loadRequest(PrivilegedRequest(url: url) as URLRequest)
        }
    }
    
    func dismissOnboarding(_ controller: OnboardingRewardsAgreementViewController,
                           state: OnboardingRewardsState) {
        Preferences.General.basicOnboardingCompleted.value = OnboardingState.completed.rawValue
        
        // Present NTP Education If Locale is JP and onboading is finished or skipped
        // Present private browsing prompt if necessary when onboarding has been skipped
        controller.dismiss(animated: true) { [weak self] in
            guard let self = self else { return }
            
            self.presentEducationNTPIfNeeded()
        }
    }
    
    // 60 days until the next time the user sees the onboarding..
    static let onboardingDaysInterval = TimeInterval(60.days)
}
