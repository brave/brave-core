// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit

/// The rounded container view hosting the URL/search bar contents, shared by `TopToolbarView` and
/// `SearchURLBarInputView`.
///
/// UIKit only supports a single shadow per layer, but the location bar's design calls for two
/// stacked shadows, so a second, background view is used to render the additional shadow behind
/// `contentView`.
class LocationContainerView: UIView {
  /// The visible container that should host the location/search bar's contents.
  let contentView = UIView().then {
    $0.backgroundColor = .clear
    $0.layer.cornerCurve = .continuous
    $0.layer.shadowOffset = .init(width: 0, height: 1)
    $0.layer.shadowRadius = 2
    $0.layer.shadowColor = UIColor.black.cgColor
    $0.layer.shadowOpacity = 0.1
  }

  // `contentView` has a second shadow but we can't apply 2 shadows on the same layer in UIKit,
  // so adding a second view behind it.
  private let secondShadowView = UIView().then {
    $0.backgroundColor = .clear
    $0.layer.cornerCurve = .continuous
    $0.layer.shadowOffset = .init(width: 0, height: 4)
    $0.layer.shadowRadius = 16
    $0.layer.shadowOpacity = 0.08
    $0.layer.shadowColor = UIColor.black.cgColor
    $0.isUserInteractionEnabled = false
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    backgroundColor = .clear
    addSubview(secondShadowView)
    addSubview(contentView)
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError()
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    let cornerRadius = 10.0

    contentView.frame = bounds
    secondShadowView.frame = bounds

    contentView.layer.cornerRadius = cornerRadius
    secondShadowView.layer.cornerRadius = cornerRadius

    contentView.layer.shadowPath =
      UIBezierPath(
        roundedRect: contentView.bounds.insetBy(dx: 1, dy: 1),  // -1 spread in Figma
        cornerRadius: cornerRadius
      ).cgPath
    secondShadowView.layer.shadowPath =
      UIBezierPath(
        roundedRect: secondShadowView.bounds.insetBy(dx: 2, dy: 2),  // -2 spread in Figma
        cornerRadius: cornerRadius
      ).cgPath
  }
}
