// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import Foundation
import Shared
import SnapKit
import UIKit

public class WelcomeBraveBlockedAdsController: UIViewController, PopoverContentComponent {

  private struct UX {
    static let detailsViewEdgeInset = UIEdgeInsets(equalInset: 15)
    static let contentViewEdgeInset = UIEdgeInsets(top: 25, left: 15, bottom: 15, right: 15.0)
    static let instructionsEdgeInset = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 15.0)
  }

  let contentStackView = BlockedAdsStackView(edgeInsets: UX.contentViewEdgeInset, spacing: 15.0)
    .then {
      $0.axis = .vertical
      $0.alignment = .fill
    }

  private let footNoteStackView = BlockedAdsStackView(
    edgeInsets: UX.detailsViewEdgeInset,
    spacing: 10.0
  )
  private let informationStackView = BlockedAdsStackView(
    edgeInsets: UX.detailsViewEdgeInset,
    spacing: 15.0
  )
  private let instructionStackView = BlockedAdsStackView(edgeInsets: UX.instructionsEdgeInset)

  private let braveIconView = UIImageView().then {
    $0.contentMode = .scaleAspectFit
    $0.image = UIImage(named: "welcome-view-ntp-logo", in: .module, compatibleWith: nil)
    $0.snp.makeConstraints {
      $0.size.equalTo(40)
    }
    $0.setContentHuggingPriority(.required, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
  }

  private let shieldIconView = UIImageView().then {
    $0.contentMode = .scaleAspectFit
    $0.image = UIImage(named: "shield-information", in: .module, compatibleWith: nil)
    $0.snp.makeConstraints {
      $0.size.equalTo(80)
    }
    $0.setContentHuggingPriority(.required, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
  }

  private let footNoteTextLabel = UILabel().then {
    $0.textColor = .bravePrimary.resolvedColor(with: .init(userInterfaceStyle: .dark))
    $0.numberOfLines = 0
    $0.text = Strings.Onboarding.blockedAdsOnboardingFootnoteText
    $0.font = .preferredFont(forTextStyle: .footnote)
    $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.setContentHuggingPriority(.defaultLow, for: .vertical)
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }

  private let informationTextLabel = UILabel().then {
    $0.textColor = .bravePrimary.resolvedColor(with: .init(userInterfaceStyle: .dark))
    $0.numberOfLines = 0
    $0.font = .preferredFont(forTextStyle: .title1)
    $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
    $0.setContentHuggingPriority(.defaultLow, for: .vertical)
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }

  private let instructionsTextLabel = UILabel().then {
    $0.textColor = .bravePrimary.resolvedColor(with: .init(userInterfaceStyle: .dark))
    $0.text = Strings.Onboarding.blockedAdsOnboardingInstructionsText
    $0.numberOfLines = 0
    $0.textAlignment = .right
    $0.font = .preferredFont(forTextStyle: .body)
    $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.setContentHuggingPriority(.defaultLow, for: .vertical)
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }

  private let numberOfTrackersTextLabel = UILabel().then {
    $0.text = Strings.Onboarding.blockedAdsOnboardingInstructionsText
    $0.textColor = .bravePrimary.resolvedColor(with: .init(userInterfaceStyle: .dark))
    $0.numberOfLines = 1
    $0.textAlignment = .left
    $0.font = UIFontMetrics(forTextStyle: .headline).scaledFont(for: .systemFont(ofSize: 96.0))
    $0.adjustsFontForContentSizeCategory = true
    $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.setContentHuggingPriority(.defaultLow, for: .vertical)
    $0.setContentCompressionResistancePriority(.required, for: .vertical)
  }

  private var gradientView = BraveGradientView.gradient01

  public override func viewDidLoad() {
    super.viewDidLoad()

    informationStackView.addBackground(color: .black.withAlphaComponent(0.1), cornerRadius: 6.0)
    footNoteStackView.addBackground(color: .black.withAlphaComponent(0.1), cornerRadius: 6.0)

    view.addSubview(gradientView)
    gradientView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    gradientView.addSubview(contentStackView)
    contentStackView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    contentStackView.addStackViewItems(
      .view(instructionStackView),
      .customSpace(15.0),
      .view(informationStackView),
      .customSpace(15.0),
      .view(footNoteStackView)
    )
  }

  public func setData(displayTrackers: [String], trackerCount: Int) {
    if displayTrackers.isEmpty {
      numberOfTrackersTextLabel.text = "\(trackerCount)"
      informationTextLabel.text = Strings.Onboarding.blockedAdsOnboardingNoBigTechInformationText

      informationStackView.addStackViewItems(
        .view(numberOfTrackersTextLabel),
        .view(informationTextLabel)
      )
    } else {
      let numBigTechTrackers = trackerCount - displayTrackers.count
      var bigTechTrackerNameList = displayTrackers.first?.capitalizeFirstLetter ?? ""
      for (index, tracker) in displayTrackers.enumerated()
      where (displayTrackers.count > 1 && index > 0) {
        bigTechTrackerNameList += ", \(tracker.capitalizeFirstLetter)"
      }

      informationTextLabel.text = String.localizedStringWithFormat(
        Strings.Onboarding.blockedAdsOnboardingBigTechInformationText,
        bigTechTrackerNameList,
        numBigTechTrackers
      )
      informationStackView.addStackViewItems(
        .view(shieldIconView),
        .view(informationTextLabel)
      )
    }

    footNoteStackView.addStackViewItems(
      .view(braveIconView),
      .view(footNoteTextLabel)
    )

    instructionStackView.addStackViewItems(
      .view(instructionsTextLabel)
    )
  }
}
