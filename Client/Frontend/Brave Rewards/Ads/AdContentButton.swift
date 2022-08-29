// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveCore
import Shared
import BraveUI

/// The main ads view. Mimics a system notification in that it shows an icon, "app name" (always will be "Brave Rewards"), title and body.
class AdContentButton: UIControl {
  let titleLabel = UILabel().then {
    $0.textColor = .bravePrimary
    $0.font = .systemFont(ofSize: 15.0, weight: .semibold)
    $0.numberOfLines = 2
  }
  let bodyLabel = UILabel().then {
    $0.textColor = .bravePrimary
    $0.font = .systemFont(ofSize: 15.0)
    $0.numberOfLines = 3
  }
  private let appNameLabel = UILabel().then {
    $0.textColor = UIColor.bravePrimary.withAlphaComponent(0.5)
    $0.font = .systemFont(ofSize: 14.0, weight: .regular)
    $0.text = Strings.Ads.adNotificationTitle.uppercased()
  }

  private let backgroundView: UIVisualEffectView = {
    let backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .systemThinMaterial))
    backgroundView.isUserInteractionEnabled = false
    backgroundView.contentView.backgroundColor = UIColor.braveBackground.withAlphaComponent(0.5)
    backgroundView.layer.cornerRadius = 10
    backgroundView.layer.cornerCurve = .continuous
    backgroundView.layer.masksToBounds = true
    return backgroundView
  }()

  override public init(frame: CGRect) {
    super.init(frame: frame)

    let headerStackView = UIStackView().then {
      $0.spacing = 8.0
    }

    let stackView = UIStackView().then {
      $0.axis = .vertical
      $0.spacing = 2.0
      $0.alignment = .leading
      $0.isUserInteractionEnabled = false
    }

    addSubview(backgroundView)
    addSubview(stackView)
    stackView.addArrangedSubview(headerStackView)
    stackView.setCustomSpacing(8, after: headerStackView)
    stackView.addArrangedSubview(titleLabel)
    stackView.addArrangedSubview(bodyLabel)

    let iconImageView = UIImageView(image: UIImage(sharedNamed: "bat-small")!)

    headerStackView.addArrangedSubview(iconImageView)
    headerStackView.addArrangedSubview(appNameLabel)

    backgroundView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }

    stackView.snp.makeConstraints {
      $0.edges.equalTo(self).inset(UIEdgeInsets(top: 8, left: 12, bottom: 8, right: 12))
    }

    backgroundColor = .clear

    layer.borderColor = UIColor.black.withAlphaComponent(0.15).cgColor
    layer.borderWidth = 1.0 / UIScreen.main.scale
    layer.cornerRadius = 10
    layer.shadowColor = UIColor.black.cgColor
    layer.shadowOpacity = 0.25
    layer.shadowOffset = CGSize(width: 0, height: 1)
    layer.shadowRadius = 2
  }

  public override func layoutSubviews() {
    super.layoutSubviews()

    layer.shadowPath = UIBezierPath(roundedRect: bounds, cornerRadius: layer.cornerRadius).cgPath
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  public override var isHighlighted: Bool {
    didSet {
      setNeedsLayout()
      let scale: CGFloat = self.isHighlighted ? 1.025 : 1
      UIViewPropertyAnimator(duration: 0.3, dampingRatio: 0.8) { [self] in
        // Only adjust scale since tx/ty could be altered
        transform.a = scale
        transform.d = scale
        layoutIfNeeded()
      }
      .startAnimation()
    }
  }
}
