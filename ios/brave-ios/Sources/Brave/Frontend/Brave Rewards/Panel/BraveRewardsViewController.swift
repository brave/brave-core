// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
import Combine
import Foundation
import Shared
import UIKit

class BraveRewardsViewController: UIViewController, PopoverContentComponent {
  enum Action {
    case unverifiedPublisherLearnMoreTapped
  }

  let browserTab: Tab
  let rewards: BraveRewards
  var actionHandler: ((Action) -> Void)?

  private var rewardsObserver: RewardsObserver?
  private var publisher: BraveCore.BraveRewards.PublisherInfo? {
    didSet {
      let isVerified = publisher?.status != .notVerified
      rewardsView.publisherView.learnMoreButton.isHidden = isVerified
      rewardsView.publisherView.hostLabel.attributedText = publisher?.attributedDisplayName(
        fontSize: BraveRewardsPublisherView.UX.hostLabelFontSize
      )
      rewardsView.publisherView.bodyLabel.text =
        isVerified ? Strings.Rewards.supportingPublisher : Strings.Rewards.unverifiedPublisher
      if !isVerified {
        rewardsView.publisherView.faviconImageView.clearMonogramFavicon()
        rewardsView.publisherView.faviconImageView.image = UIImage(
          named: "rewards-panel-unverified-pub",
          in: .module,
          compatibleWith: nil
        )!.withRenderingMode(.alwaysOriginal)
        rewardsView.publisherView.faviconImageView.contentMode = .center
      } else {
        if let url = browserTab.url {
          rewardsView.publisherView.faviconImageView.contentMode = .scaleAspectFit
          rewardsView.publisherView.faviconImageView.loadFavicon(
            for: url,
            isPrivateBrowsing: browserTab.isPrivate
          )
        } else {
          rewardsView.publisherView.faviconImageView.isHidden = true
        }
      }
    }
  }

  init(tab: Tab, rewards: BraveRewards) {
    self.browserTab = tab
    self.rewards = rewards

    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  private var rewardsView: BraveRewardsView {
    view as! BraveRewardsView
  }

  override func loadView() {
    view = BraveRewardsView()
  }

  private func reloadData() {
    guard let rewardsAPI = self.rewards.rewardsAPI else {
      self.rewardsView.statusView.setVisibleStatus(status: .rewardsOff, animated: false)
      return
    }
    if !self.rewards.isEnabled {
      self.rewardsView.statusView.setVisibleStatus(status: .rewardsOff, animated: false)
      self.rewardsView.publisherView.isHidden = true
    } else {
      if let url = self.browserTab.url, !url.isLocal, !InternalURL.isValid(url: url) {
        self.rewardsView.publisherView.isHidden = false
        self.rewardsView.publisherView.hostLabel.text = url.baseDomain
        rewardsAPI.fetchPublisherActivity(
          from: url,
          faviconURL: nil,
          publisherBlob: nil,
          tabId: UInt64(self.browserTab.rewardsId)
        )
      } else {
        self.rewardsView.publisherView.isHidden = true
      }
      self.rewardsView.statusView.setVisibleStatus(
        status: .rewardsOn,
        animated: false
      )
    }

    if let displayName = publisher?.attributedDisplayName(
      fontSize: BraveRewardsPublisherView.UX.hostLabelFontSize
    ) {
      rewardsView.publisherView.hostLabel.attributedText = displayName
    } else {
      rewardsView.publisherView.hostLabel.text = browserTab.url?.baseDomain
    }
  }

  private let rewardsServiceStartGroup = DispatchGroup()

  override func viewDidLoad() {
    super.viewDidLoad()

    rewardsView.rewardsToggle.isOn = rewards.isEnabled

    rewardsServiceStartGroup.enter()
    rewards.startRewardsService { [weak self] in
      guard let self = self else { return }
      defer { self.rewardsServiceStartGroup.leave() }
      if let rewardsAPI = self.rewards.rewardsAPI {
        let observer = RewardsObserver(rewardsAPI: rewardsAPI)
        rewardsAPI.add(observer)
        self.rewardsObserver = observer

        observer.fetchedPanelPublisher = { [weak self] publisher, tabId in
          guard let self = self else { return }
          DispatchQueue.main.async {
            if tabId == self.browserTab.rewardsId {
              self.publisher = publisher
            }
          }
        }
      }
      self.reloadData()
    }

    view.snp.makeConstraints {
      $0.width.equalTo(360)
      $0.height.equalTo(rewardsView)
    }

    rewardsView.publisherView.learnMoreButton.addTarget(
      self,
      action: #selector(tappedUnverifiedPubLearnMore),
      for: .touchUpInside
    )
    rewardsView.subtitleLabel.text =
      rewards.isEnabled ? Strings.Rewards.enabledBody : Strings.Rewards.disabledBody
    rewardsView.rewardsToggle.addTarget(
      self,
      action: #selector(rewardsToggleValueChanged),
      for: .valueChanged
    )
  }

  // MARK: - Actions

  private var isCreatingWallet: Bool = false
  @objc private func rewardsToggleValueChanged() {
    rewardsServiceStartGroup.notify(queue: .main) { [self] in
      rewardsView.rewardsToggle.isUserInteractionEnabled = false
      DispatchQueue.main.asyncAfter(deadline: .now() + 1) { [weak self] in
        self?.rewardsView.rewardsToggle.isUserInteractionEnabled = true
      }
      let isOn = rewardsView.rewardsToggle.isOn
      rewards.isEnabled = isOn
      rewardsView.subtitleLabel.text =
        isOn ? Strings.Rewards.enabledBody : Strings.Rewards.disabledBody
      if rewardsView.rewardsToggle.isOn {
        rewardsView.statusView.setVisibleStatus(
          status: .rewardsOn
        )
      } else {
        rewardsView.statusView.setVisibleStatus(status: .rewardsOff)
      }
      if publisher != nil {
        UIView.animate(withDuration: 0.15) {
          self.rewardsView.publisherView.isHidden = !self.rewardsView.rewardsToggle.isOn
          self.rewardsView.publisherView.alpha = self.rewardsView.rewardsToggle.isOn ? 1.0 : 0.0
        }
      }
    }
  }

  @objc private func tappedUnverifiedPubLearnMore() {
    actionHandler?(.unverifiedPublisherLearnMoreTapped)
  }
}
