// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Foundation
import Shared
import SnapKit
import SwiftUI
import UIKit
import Web

private struct InfoBarUX {
  static let tapDismissalMode: Toast.TapDismissalMode = .dismissOnOutsideTap
  static let padding: CGFloat = 10.0
  static let closeButtonWidth: CGFloat = 20.0
  static let labelFont = UIFont.preferredFont(forTextStyle: .subheadline)
  static let backgroundColor = UIColor(braveSystemName: .schemesOnPrimaryFixed)
}

class InfoBar: Toast, UITextViewDelegate {
  let tabManager: TabManager
  let labelText: String
  let linkText: String
  let linkUrl: String
  let onLinkPressed: (() -> Void)?
  let onClosePressed: (() -> Void)?

  init(
    tabManager: TabManager,
    labelText: String,
    linkText: String,
    linkUrl: String,
    onLinkPressed: (() -> Void)? = nil,
    onClosePressed: (() -> Void)? = nil
  ) {
    self.tabManager = tabManager
    self.labelText = labelText
    self.linkText = linkText
    self.linkUrl = linkUrl
    self.onLinkPressed = onLinkPressed
    self.onClosePressed = onClosePressed

    super.init(frame: .zero)

    self.tapDismissalMode = InfoBarUX.tapDismissalMode

    self.clipsToBounds = true

    toastView.backgroundColor = InfoBarUX.backgroundColor

    let toastContent = createToastContent(self.labelText + " ")
    toastView.addSubview(toastContent)
    toastContent.snp.makeConstraints { make in
      make.centerX.equalTo(toastView)
      make.centerY.equalTo(toastView)
      make.edges.equalTo(toastView).inset(InfoBarUX.padding)
    }

    addSubview(toastView)
    setupInitialToastConstraints()
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  fileprivate func createToastContent(
    _ labelText: String
  ) -> UIView {
    let horizontalStackView = UIStackView()
    horizontalStackView.axis = .horizontal
    horizontalStackView.alignment = .center
    horizontalStackView.spacing = InfoBarUX.padding

    let label = UITextView()
    label.textAlignment = .left
    label.textColor = .white
    label.font = InfoBarUX.labelFont
    label.backgroundColor = InfoBarUX.backgroundColor
    label.isEditable = false
    label.isScrollEnabled = false
    label.isSelectable = true
    label.delegate = self

    let learnMoreOptOutChoicesText = self.linkText
    let attributes: [NSAttributedString.Key: Any] = [
      .foregroundColor: UIColor.white,
      .font: InfoBarUX.labelFont,
    ]

    let linkAttributes: [NSAttributedString.Key: Any] = [
      .font: InfoBarUX.labelFont,
      .foregroundColor: UIColor.white,
      .underlineStyle: 1,
    ]
    label.linkTextAttributes = linkAttributes

    let nsLabelAttributedString = NSMutableAttributedString(
      string: labelText,
      attributes: attributes
    )
    let nsLinkAttributedString = NSMutableAttributedString(
      string: learnMoreOptOutChoicesText,
      attributes: linkAttributes
    )

    if let url = URL(string: self.linkUrl) {
      let linkTextRange = NSRange(location: 0, length: learnMoreOptOutChoicesText.count)
      nsLinkAttributedString.addAttribute(.link, value: url, range: linkTextRange)
      nsLabelAttributedString.append(nsLinkAttributedString)
      label.isUserInteractionEnabled = true
    }
    label.attributedText = nsLabelAttributedString
    horizontalStackView.addArrangedSubview(label)

    let button = UIButton()
    button.setImage(UIImage(braveSystemNamed: "leo.close")!, for: .normal)
    button.imageView?.contentMode = .scaleAspectFit
    button.imageView?.tintColor = .white
    button.imageView?.preferredSymbolConfiguration = .init(
      font: .preferredFont(for: .title3, weight: .regular),
      scale: .small
    )
    button.snp.makeConstraints {
      $0.width.equalTo(InfoBarUX.closeButtonWidth)
    }
    button.addTarget(self, action: #selector(buttonPressed), for: .touchUpInside)
    horizontalStackView.addArrangedSubview(button)

    return horizontalStackView
  }

  func textView(
    _ textView: UITextView,
    shouldInteractWith url: URL,
    in characterRange: NSRange,
    interaction: UITextItemInteraction
  ) -> Bool {
    self.onLinkPressed?()
    self.tabManager.addTabAndSelect(
      URLRequest(url: URL(string: self.linkUrl)!),
      isPrivate: false
    )
    dismiss(true)
    return false
  }

  @objc func buttonPressed(_ gestureRecognizer: UIGestureRecognizer) {
    self.onClosePressed?()
    dismiss(true)
  }
}
