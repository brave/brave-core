/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

/**
 * Button whose insets are included in its intrinsic size.
 */
public class InsetButton: UIButton {
  public override var intrinsicContentSize: CGSize {
    let size = super.intrinsicContentSize
    return CGSize(
      width: size.width + titleEdgeInsets.left + titleEdgeInsets.right,
      height: size.height + titleEdgeInsets.top + titleEdgeInsets.bottom)
  }

  public func addTrailingImageIcon(image: UIImage, inset: CGFloat = 15) {
    let imageView = UIImageView(image: image).then {
      $0.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)
    }

    addSubview(imageView)
    titleEdgeInsets.right += inset

    imageView.snp.makeConstraints {
      if let titleView = titleLabel {
        $0.leading.equalTo(titleView.snp.trailing).inset(-inset)
        $0.centerY.equalTo(titleView.snp.centerY)
      } else {
        $0.leading.equalToSuperview().inset(inset)
        $0.centerY.equalToSuperview()
      }

      $0.trailing.equalToSuperview().inset(inset)
      $0.width.equalTo(inset)
      $0.height.equalTo(inset)
    }
  }
}

public class SelectedInsetButton: InsetButton {
  public override var isSelected: Bool {
    didSet {
      backgroundColor = isSelected ? selectedBackgroundColor : .clear
      setTitleColor(isSelected ? .white : .braveLabel, for: .normal)
    }
  }

  public var selectedBackgroundColor: UIColor? {
    didSet {
      if isSelected {
        backgroundColor = selectedBackgroundColor
      }
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    contentEdgeInsets = UIEdgeInsets(top: 3, left: 6, bottom: 3, right: 6)
    layer.cornerRadius = 4.0
    layer.cornerCurve = .continuous

    setTitleColor(.braveLabel, for: .normal)
    selectedBackgroundColor = .braveBlurpleTint
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}
