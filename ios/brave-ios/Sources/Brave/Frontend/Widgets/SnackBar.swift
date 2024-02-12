/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SnapKit
import Shared
import UIKit
import BraveUI
import Favicon

class SnackBarUX {
  static var maxWidth: CGFloat = 400
  static let borderWidth: CGFloat = 1.0 / UIScreen.main.scale
  static let highlightColor = UIColor.braveInfoBorder.withAlphaComponent(0.9)
}

/**
 * A specialized version of UIButton for use in SnackBars. These are displayed evenly
 * spaced in the bottom of the bar. The main convenience of these is that you can pass
 * in a callback in the constructor (although these also style themselves appropriately).
 */
typealias SnackBarCallback = (_ bar: SnackBar) -> Void
class SnackButton: UIButton {
  let callback: SnackBarCallback?
  fileprivate var bar: SnackBar!

  init(title: String, accessibilityIdentifier: String, callback: @escaping SnackBarCallback) {
    self.callback = callback

    super.init(frame: .zero)

    setTitle(title, for: .normal)
    titleLabel?.font = DynamicFontHelper.defaultHelper.DefaultMediumFont
    setTitleColor(.braveBlurpleTint, for: .highlighted)
    setTitleColor(.braveLabel, for: .normal)
    addTarget(self, action: #selector(onClick), for: .touchUpInside)
    self.accessibilityIdentifier = accessibilityIdentifier
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  @objc func onClick() {
    callback?(bar)
  }

  func drawSeparator() {
    let separator = UIView()
    separator.backgroundColor = .braveSeparator
    self.addSubview(separator)
    separator.snp.makeConstraints { make in
      make.leading.equalTo(self)
      make.width.equalTo(SnackBarUX.borderWidth)
      make.top.bottom.equalTo(self)
    }
  }

}

class SnackBar: UIView {
  let backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .systemChromeMaterial))

  private lazy var imageView: UIImageView = {
    let imageView = UIImageView()
    imageView.contentMode = .scaleAspectFit
    // These are requried to make sure that the image is _never_ smaller or larger than its actual size
    imageView.setContentHuggingPriority(.required, for: .horizontal)
    imageView.setContentHuggingPriority(.required, for: .vertical)
    imageView.setContentCompressionResistancePriority(.required, for: .horizontal)
    imageView.setContentCompressionResistancePriority(.required, for: .vertical)
    return imageView
  }()

  private lazy var textLabel: UILabel = {
    let label = UILabel()
    label.font = DynamicFontHelper.defaultHelper.DefaultMediumFont
    label.lineBreakMode = .byWordWrapping
    label.setContentCompressionResistancePriority(.required, for: .horizontal)
    label.backgroundColor = nil
    label.numberOfLines = 0
    label.textColor = .braveLabel
    label.backgroundColor = .clear
    return label
  }()

  private lazy var buttonsView: UIStackView = {
    let stack = UIStackView()
    stack.distribution = .fillEqually
    return stack
  }()

  private lazy var titleView: UIStackView = {
    let stack = UIStackView()
    stack.spacing = UIConstants.defaultPadding
    stack.distribution = .fill
    stack.axis = .horizontal
    stack.alignment = .center
    return stack
  }()

  // The Constraint for the bottom of this snackbar. We use this to transition it
  var bottom: Constraint?

  init(text: String, img: UIImage?) {
    super.init(frame: .zero)

    imageView.image = img ?? Favicon.defaultImage
    textLabel.text = text
    setup()
  }

  fileprivate func setup() {
    addSubview(backgroundView)
    titleView.addArrangedSubview(imageView)
    titleView.addArrangedSubview(textLabel)

    let separator = UIView()
    separator.backgroundColor = .braveSeparator

    addSubview(titleView)
    addSubview(separator)
    addSubview(buttonsView)

    separator.snp.makeConstraints { make in
      make.leading.trailing.equalTo(self)
      make.height.equalTo(SnackBarUX.borderWidth)
      make.top.equalTo(buttonsView.snp.top).offset(-1)
    }

    backgroundView.snp.makeConstraints { make in
      make.bottom.left.right.equalTo(self)
      // Offset it by the width of the top border line so we can see the line from the super view
      make.top.equalTo(self).offset(1)
    }

    titleView.snp.makeConstraints { make in
      make.top.equalTo(self).offset(UIConstants.defaultPadding)
      make.centerX.equalTo(self).priority(500)
      make.width.lessThanOrEqualTo(self).inset(UIConstants.defaultPadding * 2).priority(1000)
    }

    backgroundColor = .clear
    layer.borderWidth = SnackBarUX.borderWidth
    layer.borderColor = UIColor.braveSeparator.resolvedColor(with: traitCollection).cgColor
    layer.cornerRadius = 6
    layer.cornerCurve = .continuous
    clipsToBounds = true
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)

    self.layer.borderColor = UIColor.braveSeparator.resolvedColor(with: traitCollection).cgColor
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  /**
     * Called to check if the snackbar should be removed or not. By default, Snackbars persist forever.
     * Override this class or use a class like CountdownSnackbar if you want things expire
     * - returns: true if the snackbar should be kept alive
     */
  func shouldPersist(_ tab: Tab) -> Bool {
    return true
  }

  override func updateConstraints() {
    super.updateConstraints()

    buttonsView.snp.remakeConstraints { make in
      make.top.equalTo(titleView.snp.bottom).offset(UIConstants.defaultPadding)
      make.bottom.equalTo(self.snp.bottom)
      make.leading.trailing.equalTo(self)
      if !buttonsView.subviews.isEmpty {
        make.height.equalTo(UIConstants.snackbarButtonHeight)
      } else {
        make.height.equalTo(0)
      }
    }
  }

  var showing: Bool {
    return alpha != 0 && self.superview != nil
  }

  func show() {
    alpha = 1
    bottom?.update(offset: 0)
  }

  func addButton(_ snackButton: SnackButton) {
    snackButton.bar = self
    buttonsView.addArrangedSubview(snackButton)

    // Only show the separator on the left of the button if it is not the first view
    if buttonsView.arrangedSubviews.count != 1 {
      snackButton.drawSeparator()
    }
  }
}

/**
 * A special version of a snackbar that persists for at least a timeout. After that
 * it will dismiss itself on the next page load where this tab isn't showing. As long as
 * you stay on the current tab though, it will persist until you interact with it.
 */
class TimerSnackBar: SnackBar {
  fileprivate var timer: Timer?
  fileprivate var timeout: TimeInterval

  init(timeout: TimeInterval = 10, text: String, img: UIImage?) {
    self.timeout = timeout
    super.init(text: text, img: img)
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  static func showAppStoreConfirmationBar(forTab tab: Tab, appStoreURL: URL) {
    let bar = TimerSnackBar(text: Strings.externalLinkAppStoreConfirmationTitle, img: Favicon.defaultImage)
    let openAppStore = SnackButton(title: Strings.OKString, accessibilityIdentifier: "ConfirmOpenInAppStore") { bar in
      tab.removeSnackbar(bar)
      UIApplication.shared.open(appStoreURL)
    }
    let cancelButton = SnackButton(title: Strings.cancelButtonTitle, accessibilityIdentifier: "CancelOpenInAppStore") { bar in
      tab.removeSnackbar(bar)
    }
    bar.addButton(openAppStore)
    bar.addButton(cancelButton)
    tab.addSnackbar(bar)
  }

  override func show() {
    self.timer = Timer(timeInterval: timeout, target: self, selector: #selector(timerDone), userInfo: nil, repeats: false)
    RunLoop.current.add(self.timer!, forMode: .default)
    super.show()
  }

  @objc func timerDone() {
    self.timer = nil
  }

  override func shouldPersist(_ tab: Tab) -> Bool {
    if !showing {
      return timer != nil
    }
    return super.shouldPersist(tab)
  }
}
