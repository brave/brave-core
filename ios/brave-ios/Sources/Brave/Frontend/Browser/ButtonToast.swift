// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SnapKit
import UIKit

struct ButtonToastUX {
  static let toastHeight: CGFloat = 55.0
  static let toastPadding: CGFloat = 10.0
  static let toastButtonPadding: CGFloat = 10.0
  static let toastDelay = DispatchTimeInterval.milliseconds(900)
  static let toastButtonBorderRadius: CGFloat = 5
  static let toastButtonBorderWidth: CGFloat = 1
  static let toastLabelFont = UIFont.systemFont(ofSize: 15, weight: .semibold)
  static let toastDescriptionFont = UIFont.systemFont(ofSize: 13)
  static let toastDismissAfter = DispatchTimeInterval.seconds(10)
}

class ButtonToast: Toast {
  class HighlightableButton: UIButton {
    override var isHighlighted: Bool {
      didSet {
        backgroundColor = isHighlighted ? .white : .clear
      }
    }
  }

  init(
    labelText: String,
    descriptionText: String? = nil,
    image: UIImage? = nil,
    buttonText: String? = nil,
    backgroundColor: UIColor = SimpleToastUX.toastDefaultColor,
    textAlignment: NSTextAlignment = .left,
    completion: ((_ buttonPressed: Bool) -> Void)? = nil
  ) {
    super.init(frame: .zero)

    self.completionHandler = completion

    self.clipsToBounds = true
    self.addSubview(
      createView(
        labelText,
        descriptionText: descriptionText,
        image: image,
        buttonText: buttonText,
        textAlignment: textAlignment
      )
    )

    self.toastView.backgroundColor = backgroundColor

    self.toastView.snp.makeConstraints { make in
      make.left.right.height.equalTo(self)
      self.animationConstraint = make.top.equalTo(self.snp.bottom).constraint
    }

    self.snp.makeConstraints { make in
      make.height.equalTo(ButtonToastUX.toastHeight)
    }
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  fileprivate func createView(
    _ labelText: String,
    descriptionText: String?,
    image: UIImage?,
    buttonText: String?,
    textAlignment: NSTextAlignment
  ) -> UIView {
    let horizontalStackView = UIStackView()
    horizontalStackView.axis = .horizontal
    horizontalStackView.alignment = .center
    horizontalStackView.spacing = ButtonToastUX.toastPadding

    if let image = image {
      let icon = UIImageView(image: image)
      icon.tintColor = .white
      horizontalStackView.addArrangedSubview(icon)
    }

    let labelStackView = UIStackView()
    labelStackView.axis = .vertical
    labelStackView.alignment = .leading

    let label = UILabel()
    label.textAlignment = textAlignment
    label.textColor = .white
    label.font = ButtonToastUX.toastLabelFont
    label.text = labelText
    label.lineBreakMode = .byWordWrapping
    label.numberOfLines = 0
    labelStackView.addArrangedSubview(label)

    var descriptionLabel: UILabel?
    if let descriptionText = descriptionText {
      label.lineBreakMode = .byClipping
      label.numberOfLines = 1  // if showing a description we cant wrap to the second line
      label.adjustsFontSizeToFitWidth = true

      descriptionLabel = UILabel()
      descriptionLabel?.textAlignment = textAlignment
      descriptionLabel?.textColor = .white
      descriptionLabel?.font = ButtonToastUX.toastDescriptionFont
      descriptionLabel?.text = descriptionText
      descriptionLabel?.lineBreakMode = .byTruncatingTail
      labelStackView.addArrangedSubview(descriptionLabel!)
    }

    horizontalStackView.addArrangedSubview(labelStackView)

    if let buttonText = buttonText {
      let button = HighlightableButton()
      button.layer.cornerRadius = ButtonToastUX.toastButtonBorderRadius
      button.layer.cornerCurve = .continuous
      button.layer.borderWidth = ButtonToastUX.toastButtonBorderWidth
      button.layer.borderColor = UIColor.white.cgColor
      button.setTitle(buttonText, for: [])
      button.setTitleColor(toastView.backgroundColor, for: .highlighted)
      button.titleLabel?.font = SimpleToastUX.toastFont
      button.titleLabel?.numberOfLines = 1
      button.titleLabel?.lineBreakMode = .byClipping
      button.titleLabel?.adjustsFontSizeToFitWidth = true
      button.titleLabel?.minimumScaleFactor = 0.1
      button.addGestureRecognizer(
        UITapGestureRecognizer(target: self, action: #selector(buttonPressed))
      )

      button.snp.makeConstraints { (make) in
        make.width.equalTo(
          button.titleLabel!.intrinsicContentSize.width + 2 * ButtonToastUX.toastButtonPadding
        )
      }

      horizontalStackView.addArrangedSubview(button)
    }

    toastView.addSubview(horizontalStackView)

    if textAlignment == .center {
      label.snp.makeConstraints { make in
        make.centerX.equalTo(toastView)
      }

      descriptionLabel?.snp.makeConstraints { make in
        make.centerX.equalTo(toastView)
      }
    }

    horizontalStackView.snp.makeConstraints { make in
      make.centerX.equalTo(toastView)
      make.centerY.equalTo(toastView)
      make.width.equalTo(toastView.snp.width).offset(-2 * ButtonToastUX.toastPadding)
    }

    return toastView
  }

  @objc func buttonPressed(_ gestureRecognizer: UIGestureRecognizer) {
    completionHandler?(true)
    dismiss(true)
  }

  override func showToast(
    viewController: UIViewController? = nil,
    delay: DispatchTimeInterval = SimpleToastUX.toastDelayBefore,
    duration: DispatchTimeInterval? = SimpleToastUX.toastDismissAfter,
    makeConstraints: @escaping (ConstraintMaker) -> Void,
    completion: (() -> Void)? = nil
  ) {
    super.showToast(
      viewController: viewController,
      delay: delay,
      duration: duration,
      makeConstraints: makeConstraints
    )
  }
}
