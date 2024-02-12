// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import BraveUI

class SimpleShieldsView: UIView {

  let faviconImageView = UIImageView().then {
    $0.snp.makeConstraints {
      $0.size.equalTo(24)
    }
    $0.layer.cornerRadius = 4
    $0.layer.cornerCurve = .continuous
    $0.clipsToBounds = true
  }

  let hostLabel = UILabel().then {
    $0.font = .systemFont(ofSize: 25.0)
    $0.textColor = .bravePrimary
  }

  let shieldsSwitch = ShieldsSwitch().then {
    $0.offBackgroundColor = .secondaryBraveBackground
  }

  private let braveShieldsLabel = UILabel().then {
    $0.text = Strings.Shields.statusTitle
    $0.font = .systemFont(ofSize: 16, weight: .medium)
    $0.textColor = .braveLabel
  }

  let statusLabel = UILabel().then {
    $0.font = .systemFont(ofSize: 16, weight: .bold)
    $0.text = Strings.Shields.statusValueUp.uppercased()
    $0.textColor = .braveLabel
  }

  let blockCountView = BlockCountView()

  let footerLabel = UILabel().then {
    $0.text = Strings.Shields.siteBroken
    $0.font = .systemFont(ofSize: 13.0)
    $0.textColor = UIColor(rgb: 0x868e96)
    $0.numberOfLines = 0
  }

  // Shields Down

  let shieldsDownStackView = UIStackView().then {
    $0.axis = .vertical
    $0.alignment = .center
    $0.spacing = 16
    $0.layoutMargins = UIEdgeInsets(top: 0, left: 24, bottom: 0, right: 24)
    $0.isLayoutMarginsRelativeArrangement = true
  }

  private let shieldsDownDisclaimerLabel = UILabel().then {
    $0.font = .systemFont(ofSize: 13)
    $0.text = Strings.Shields.shieldsDownDisclaimer
    $0.numberOfLines = 0
    $0.textAlignment = .center
    $0.textColor = .braveLabel
  }

  let reportSiteButton = ActionButton(type: .system).then {
    $0.titleLabel?.font = .systemFont(ofSize: 16, weight: .semibold)
    $0.titleEdgeInsets = UIEdgeInsets(top: 4, left: 20, bottom: 4, right: 20)
    $0.setTitle(Strings.Shields.reportABrokenSite, for: .normal)
    $0.tintColor = .braveLabel
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    let stackView = UIStackView().then {
      $0.axis = .vertical
      $0.spacing = 16
      $0.alignment = .center
      $0.layoutMargins = UIEdgeInsets(top: 24, left: 12, bottom: 24, right: 12)
      $0.isLayoutMarginsRelativeArrangement = true
    }

    addSubview(stackView)
    stackView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }

    [shieldsDownDisclaimerLabel, reportSiteButton].forEach(shieldsDownStackView.addArrangedSubview)

    stackView.addStackViewItems(
      .view(
        UIStackView(arrangedSubviews: [faviconImageView, hostLabel]).then {
          $0.spacing = 8
          $0.alignment = .center
          $0.layoutMargins = UIEdgeInsets(top: 0, left: 0, bottom: 20, right: 0)
          $0.isLayoutMarginsRelativeArrangement = true
        }),
      .view(shieldsSwitch),
      .view(
        UIStackView(arrangedSubviews: [braveShieldsLabel, statusLabel]).then {
          $0.spacing = 4
          $0.alignment = .center
        }),
      .customSpace(32),
      .view(blockCountView),
      .view(footerLabel),
      .view(shieldsDownStackView)
    )
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}

// MARK: - BlockCountView

extension SimpleShieldsView {
  
  class BlockCountView: UIView {
    
    private struct UX {
      static let descriptionEdgeInset = UIEdgeInsets(top: 13, left: 16, bottom: 13, right: 16)
      static let iconEdgeInset = UIEdgeInsets(top: 22, left: 14, bottom: 22, right: 14)
      static let hitBoxEdgeInsets = UIEdgeInsets(equalInset: -10)
      static let buttonEdgeInsets = UIEdgeInsets(top: -3, left: 4, bottom: -3, right: 4)
    }

    let contentStackView = UIStackView().then {
      $0.spacing = 2
    }

    let descriptionStackView = ShieldsStackView(edgeInsets: UX.descriptionEdgeInset).then {
      $0.spacing = 16
    }

    let infoStackView = ShieldsStackView(edgeInsets: UX.iconEdgeInset)

    let shareStackView = ShieldsStackView(edgeInsets: UX.iconEdgeInset)

    let countLabel = UILabel().then {
      $0.font = .systemFont(ofSize: 36)
      $0.setContentHuggingPriority(.required, for: .horizontal)
      $0.setContentCompressionResistancePriority(.required, for: .horizontal)
      $0.textColor = .braveLabel
    }

    private lazy var descriptionLabel = ViewLabel().then {
      $0.attributedText = {
        let string = NSMutableAttributedString(
          string: Strings.Shields.blockedCountLabel,
          attributes: [.font: UIFont.systemFont(ofSize: 13.0)]
        )
        return string
      }()
      $0.backgroundColor = .clear

      $0.isAccessibilityElement = false
      $0.textColor = .braveLabel
    }

    let infoButton = BraveButton().then {
      $0.setImage(UIImage(named: "shields-help", in: .module, compatibleWith: nil)!.template, for: .normal)
      $0.hitTestSlop = UX.hitBoxEdgeInsets
      $0.imageEdgeInsets = .zero
      $0.titleEdgeInsets = .zero
      $0.contentEdgeInsets = UIEdgeInsets(top: -2, left: 4, bottom: -3, right: 4)
      $0.contentMode = .scaleAspectFit
      $0.setContentCompressionResistancePriority(.required, for: .horizontal)
      $0.accessibilityLabel = Strings.Shields.aboutBraveShieldsTitle
      $0.tintColor = .bravePrimary
    }

    let shareButton = BraveButton().then {
      $0.setImage(UIImage(sharedNamed: "shields-share")!.template, for: .normal)
      $0.hitTestSlop = UX.hitBoxEdgeInsets
      $0.imageEdgeInsets = .zero
      $0.titleEdgeInsets = .zero
      $0.contentEdgeInsets = UIEdgeInsets(top: -2, left: 4, bottom: -3, right: 4)
      $0.contentMode = .scaleAspectFit
      $0.setContentCompressionResistancePriority(.required, for: .horizontal)
      $0.accessibilityLabel = Strings.share
      $0.tintColor = .bravePrimary
    }

    override init(frame: CGRect) {
      super.init(frame: frame)

      isAccessibilityElement = true
      accessibilityTraits.insert(.button)
      accessibilityHint = Strings.Shields.blockedInfoButtonAccessibilityLabel

      descriptionStackView.addBackground(color: .secondaryBraveBackground, cornerRadius: 6.0)
      infoStackView.addBackground(color: .secondaryBraveBackground, cornerRadius: 6.0)
      shareStackView.addBackground(color: .secondaryBraveBackground, cornerRadius: 6.0)

      addSubview(contentStackView)

      contentStackView.addStackViewItems(
        .view(descriptionStackView),
        .view(infoStackView),
        .view(shareStackView)
      )

      descriptionStackView.addStackViewItems(
        .view(countLabel),
        .view(descriptionLabel)
      )

      infoStackView.addArrangedSubview(infoButton)
      shareStackView.addArrangedSubview(shareButton)

      contentStackView.snp.makeConstraints {
        $0.edges.equalToSuperview()
      }
    }

    override var accessibilityLabel: String? {
      get {
        [countLabel.accessibilityLabel, Strings.Shields.blockedCountLabel]
          .compactMap { $0 }
          .joined(separator: " ")
      }
      set { assertionFailure() }  // swiftlint:disable:this unused_setter_value
    }

    override func accessibilityActivate() -> Bool {
      infoButton.sendActions(for: .touchUpInside)
      return true
    }

    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
  }
}

// MARK: - ShieldsStackView

extension SimpleShieldsView {

  class ShieldsStackView: UIStackView {

    init(edgeInsets: UIEdgeInsets) {
      super.init(frame: .zero)

      alignment = .center
      layoutMargins = edgeInsets
      isLayoutMarginsRelativeArrangement = true
    }

    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }

    /// Adds Background to StackView with Color and Corner Radius
    public func addBackground(color: UIColor, cornerRadius: CGFloat? = nil) {
      let backgroundView = UIView(frame: bounds).then {
        $0.backgroundColor = color
        $0.autoresizingMask = [.flexibleWidth, .flexibleHeight]
      }

      if let radius = cornerRadius {
        backgroundView.layer.cornerRadius = radius
        backgroundView.layer.cornerCurve = .continuous
      }

      insertSubview(backgroundView, at: 0)
    }
  }
}
