// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Preferences
import SafariServices
import Shared
import UIKit

protocol NTPLearnMoreViewDelegate: AnyObject {
  func learnMoreTapped()
  func hideSponsoredImagesTapped()
  func tosTapped()
}

/// A view controller that is presented after user taps on 'Learn more' on one of `NTPNotificationViewController` views.
class NTPLearnMoreViewController: BottomSheetViewController {

  var linkHandler: ((URL) -> Void)?

  private let state: BrandedImageCalloutState
  private let rewards: BraveRewards

  private let termsOfServiceUrl = "https://www.brave.com/terms_of_use"
  private let learnMoreAboutBraveRewardsUrl = "https://brave.com/brave-rewards/"

  init(state: BrandedImageCalloutState, rewards: BraveRewards) {
    self.state = state
    self.rewards = rewards
    super.init()
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    guard let mainView = mainView else {
      assertionFailure()
      return
    }

    mainView.delegate = self

    contentView.addSubview(mainView)
    mainView.snp.remakeConstraints {
      $0.top.equalToSuperview().inset(28)
      $0.leading.trailing.equalToSuperview()
      $0.bottom.equalTo(view.safeAreaLayoutGuide).inset(16)
    }
  }

  // MARK: - View setup

  private var mainView: NTPLearnMoreContentView? {
    var config: NTPNotificationLearnMoreViewConfig?

    switch state {
    case .brandedImageSupport:
      config = NTPNotificationLearnMoreViewConfig(
        headerText: Strings.NTP.sponsoredImageDescription,
        tosText: false,
        learnMoreButtonText: Strings.NTP.learnMoreAboutSI,
        headerBodySpacing: 8
      )
    case .dontShow:
      assertionFailure()
      return nil
    }

    guard let viewConfig = config else { return nil }

    return NTPLearnMoreContentView(config: viewConfig)
  }
}

// MARK: - NTPLearnMoreDelegate
extension NTPLearnMoreViewController: NTPLearnMoreViewDelegate {
  func learnMoreTapped() {
    guard let url = URL(string: learnMoreAboutBraveRewardsUrl) else { return }

    // Normal case, open link in current tab and close the modal.
    linkHandler?(url)
    self.close()
  }

  func hideSponsoredImagesTapped() {
    Preferences.NewTabPage.backgroundMediaType = .defaultImages
    self.close()
  }

  func tosTapped() {
    guard let url = URL(string: termsOfServiceUrl) else { return }
    self.showSFSafariViewController(url: url)
  }

  private func showSFSafariViewController(url: URL) {
    let config = SFSafariViewController.Configuration()

    let vc = SFSafariViewController(url: url, configuration: config)
    vc.modalPresentationStyle = .overFullScreen

    self.present(vc, animated: true)
  }
}
