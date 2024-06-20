// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import Foundation
import Lottie
import Shared
import Then
import UIKit

public enum OptInCardAction {
  case closedButtonTapped
  case turnOnBraveNewsButtonTapped
  case learnMoreButtonTapped
}

public class BraveNewsOptInView: UIView, FeedCardContent {
  private let backgroundView = FeedCardBackgroundView()

  private let stackView = UIStackView().then {
    $0.axis = .vertical
    $0.alignment = .center
    $0.spacing = 16
  }

  public let graphicAnimationView = LottieAnimationView(
    name: "brave-today-welcome-graphic",
    bundle: .module
  ).then {
    $0.contentMode = .scaleAspectFit
    $0.loopMode = .loop
  }

  public var optInCardActionHandler: ((OptInCardAction) -> Void)?

  private let closeButton = UIButton(type: .system).then {
    $0.setImage(
      UIImage(named: "card_close", in: .module, compatibleWith: nil)!.withRenderingMode(
        .alwaysOriginal
      ),
      for: .normal
    )
    $0.accessibilityLabel = Strings.close
  }

  public let turnOnBraveNewsButton = ActionButton().then {
    $0.layer.borderWidth = 0
    $0.titleLabel?.font = .systemFont(ofSize: 16.0, weight: .semibold)
    $0.setTitleColor(.white, for: .normal)
    $0.setTitle(Strings.BraveNews.turnOnBraveNews, for: .normal)
    $0.contentEdgeInsets = UIEdgeInsets(top: 10, left: 20, bottom: 10, right: 20)
    $0.backgroundColor = .braveLighterBlurple
    $0.loaderView = LoaderView(size: .small).then {
      $0.tintColor = .white
    }
  }

  private let learnMoreButton = UIButton(type: .system).then {
    $0.setTitle(Strings.BraveNews.learnMoreTitle, for: .normal)
    $0.titleLabel?.font = .systemFont(ofSize: 15.0, weight: .semibold)
    $0.setTitleColor(.white, for: .normal)
    if #available(iOS 17.0, *) {
      $0.hoverStyle = .init(
        effect: .highlight,
        shape: .capsule.inset(by: .init(top: 0, left: -8, bottom: 0, right: -8))
      )
    }
  }

  public required init() {
    super.init(frame: .zero)

    addSubview(backgroundView)
    addSubview(stackView)
    addSubview(closeButton)

    backgroundView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    stackView.snp.makeConstraints {
      $0.edges.equalToSuperview().inset(24)
    }
    stackView.addStackViewItems(
      .view(graphicAnimationView),
      .customSpace(30),
      .view(
        UILabel().then {
          $0.text = Strings.BraveNews.introCardTitle
          $0.textAlignment = .center
          $0.textColor = .white
          $0.font = .systemFont(ofSize: 18, weight: .semibold)
          $0.numberOfLines = 0
        }
      ),
      .view(
        UILabel().then {
          $0.text = Strings.BraveNews.introCardBody
          $0.textAlignment = .center
          $0.textColor = .white
          $0.font = .systemFont(ofSize: 14)
          $0.numberOfLines = 0
        }
      ),
      .customSpace(24),
      .view(turnOnBraveNewsButton),
      .view(learnMoreButton)
    )

    closeButton.snp.makeConstraints {
      $0.top.right.equalToSuperview().inset(8)
    }

    closeButton.addTarget(self, action: #selector(tappedCloseButton), for: .touchUpInside)
    learnMoreButton.addTarget(self, action: #selector(tappedLearnMoreButton), for: .touchUpInside)
    turnOnBraveNewsButton.addTarget(
      self,
      action: #selector(tappedTurnOnBraveButton),
      for: .touchUpInside
    )
  }

  // MARK: - Actions

  @objc private func tappedCloseButton() {
    optInCardActionHandler?(.closedButtonTapped)
  }

  @objc private func tappedLearnMoreButton() {
    optInCardActionHandler?(.learnMoreButtonTapped)
  }

  @objc private func tappedTurnOnBraveButton() {
    optInCardActionHandler?(.turnOnBraveNewsButtonTapped)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  // MARK: - FeedCardContent

  public var actionHandler: ((Int, FeedItemAction) -> Void)? {
    didSet {
      assertionFailure("Unused for welcome card")
    }
  }
  public var contextMenu: FeedItemMenu? {
    didSet {
      assertionFailure("Unused for welcome card")
    }
  }
}
