// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import Preferences
import BraveCore

class NTPNotificationViewController: TranslucentBottomSheet {

  private let state: BrandedImageCalloutState

  var learnMoreHandler: (() -> Void)?

  private var rewards: BraveRewards?

  init?(state: BrandedImageCalloutState, rewards: BraveRewards?) {
    self.state = state
    self.rewards = rewards
    super.init()

    if state == .dontShow { return nil }
  }

  private var mainView: UIStackView?

  override func viewDidLoad() {
    super.viewDidLoad()

    guard let mainView = createViewFromState() else {
      assertionFailure()
      return
    }

    self.mainView = mainView

    mainView.setCustomSpacing(0, after: mainView.header)
    view.addSubview(mainView)
    view.bringSubviewToFront(closeButton)
  }

  override func viewDidLayoutSubviews() {
    updateMainViewConstraints()

    view.snp.remakeConstraints {
      if isPortraitPhone {
        $0.right.left.equalToSuperview()
      } else {
        let width = min(view.frame.width, 400)
        $0.width.equalTo(width)
        $0.centerX.equalToSuperview()
      }
      $0.bottom.equalToSuperview()
    }
  }

  private func updateMainViewConstraints() {
    guard let mainView = mainView else { return }

    mainView.snp.remakeConstraints {
      $0.leading.trailing.bottom.top.equalTo(view.safeAreaLayoutGuide).inset(16)
    }
  }

  private var isPortraitPhone: Bool {
    traitCollection.userInterfaceIdiom == .phone
      && UIApplication.shared.statusBarOrientation.isPortrait
  }

  override func close(immediately: Bool = false) {
    Preferences.NewTabPage.brandedImageShowed.value = true
    super.close(immediately: immediately)
  }

  private func createViewFromState() -> NTPNotificationView? {
    var config = NTPNotificationViewConfig(textColor: .white)

    switch state {
    case .brandedImageSupport:
      let learnMore = Strings.learnMore.withNonBreakingSpace

      config.bodyText =
        (
          text: "\(Strings.NTP.sponsoredImageDescription) \(learnMore)",
          urlInfo: [learnMore: "learn-more"],
          action: { [weak self] action in
            self?.learnMoreHandler?()
            self?.close()
          }
        )
    case .dontShow:
      return nil
    }

    return NTPNotificationView(config: config)
  }
}
