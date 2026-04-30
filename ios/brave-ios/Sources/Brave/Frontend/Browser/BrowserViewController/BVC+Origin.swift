// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStore
import Onboarding
import Origin
import Preferences
import Shared
import SwiftUI

extension BrowserViewController {
  // Execute any additional code to be executed after an Origin purchase completes
  func handleOriginPurchaseCompleted() {
    PrivacyReportsManager.cancelNotification()
    BrowserViewController.cancelScheduleDefaultBrowserNotification()
  }

  func presentBraveOriginDeepLink() {
    // Ensure Onboarding is complete before handling this
    if Preferences.Onboarding.basicOnboardingCompleted.value != OnboardingState.completed.rawValue {
      return
    }

    Task {
      guard FeatureList.kBraveOrigin.enabled,
        let originService = BraveOriginServiceFactory.get(profile: profileController.profile),
        let skusService = Skus.SkusServiceFactory.get(profile: profileController.profile)
      else {
        return
      }
      let isPurchased = await originService.checkPurchaseState()
      let originSettingsController: () -> UIViewController = {
        // Origin settings is typically presented from inside settings, so we need to:
        //   1. Set up a UINavigationController container
        //   2. Add a done button to the toolbar
        let controller = UIHostingController(
          rootView: OriginSettingsView(
            viewModel: .init(
              service: originService,
              storeSDK: BraveStoreSDK(skusService: skusService)
            )
          )
          .environment(
            \.openURL,
            OpenURLAction { [weak self] url in
              guard let self else { return .handled }
              openURLInNewTab(
                url,
                isPrivate: privateBrowsingManager.isPrivateBrowsing,
                isPrivileged: url.scheme == InternalURL.scheme
              )
              dismiss(animated: true)
              return .handled
            }
          )
        )
        controller.title = Strings.Origin.originProductName  // Not Translated
        controller.navigationItem.rightBarButtonItem = .init(
          systemItem: .done,
          primaryAction: .init(handler: { [unowned self] _ in
            dismiss(animated: true)
          })
        )
        return UINavigationController(rootViewController: controller)
      }
      if isPurchased {
        present(originSettingsController(), animated: true)
      } else {
        let controller = UIHostingController(
          rootView: OriginPaywallView(
            viewModel: .init(store: .init(skusService: skusService)),
            didPurchase: { [weak self] in
              guard let self else { return }
              handleOriginPurchaseCompleted()
              present(originSettingsController(), animated: true)
            }
          )
          .environment(
            \.openURL,
            OpenURLAction { [weak self] url in
              guard let self else { return .handled }
              openURLInNewTab(
                url,
                isPrivate: privateBrowsingManager.isPrivateBrowsing,
                isPrivileged: url.scheme == InternalURL.scheme
              )
              return .handled
            }
          )
        )
        present(controller, animated: true)
      }
    }
  }
}
