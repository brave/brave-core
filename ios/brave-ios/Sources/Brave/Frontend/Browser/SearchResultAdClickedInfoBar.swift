// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Foundation
import Shared
import SnapKit
import SwiftUI
import UIKit

private struct SearchResultAdClickedInfoBarUX {
  static let toastPadding: CGFloat = 10.0
  static let toastCloseButtonWidth: CGFloat = 20.0
  static let toastLabelFont = UIFont.preferredFont(forTextStyle: .subheadline)
  static let toastBackgroundColor = UIColor(braveSystemName: .schemesOnPrimaryFixed)
  static let learnMoreUrl = "https://search.brave.com/help/conversion-reporting"
}

class SearchResultAdClickedInfoBar: Toast, UITextViewDelegate {
  let tabManager: TabManager

  init(tabManager: TabManager) {
    self.tabManager = tabManager

    super.init(frame: .zero)

    self.tapDismissalMode = .outsideTap

    self.clipsToBounds = true

    toastView.backgroundColor = SearchResultAdClickedInfoBarUX.toastBackgroundColor

    let toastContent = createToastContent(
      Strings.searchResultAdClickedInfoBarTitle + " "
    )
    toastView.addSubview(toastContent)
    toastContent.snp.makeConstraints { make in
      make.centerX.equalTo(toastView)
      make.centerY.equalTo(toastView)
      make.edges.equalTo(toastView).inset(SearchResultAdClickedInfoBarUX.toastPadding)
    }

    addSubview(toastView)
    toastView.snp.makeConstraints { make in
      make.left.right.height.equalTo(self)
      self.animationConstraint = make.top.equalTo(self.snp.bottom).constraint
    }
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
    horizontalStackView.spacing = SearchResultAdClickedInfoBarUX.toastPadding

    let label = UITextView()
    label.textAlignment = .left
    label.textColor = .white
    label.font = SearchResultAdClickedInfoBarUX.toastLabelFont
    label.backgroundColor = SearchResultAdClickedInfoBarUX.toastBackgroundColor
    label.isEditable = false
    label.isScrollEnabled = false
    label.isSelectable = true
    label.delegate = self

    let learnMoreOptOutChoicesText = Strings.searchResultAdClickedLearnMoreOptOutChoicesLabel
    let attributes: [NSAttributedString.Key: Any] = [
      .foregroundColor: UIColor.white,
      .font: SearchResultAdClickedInfoBarUX.toastLabelFont,
    ]

    let linkAttributes: [NSAttributedString.Key: Any] = [
      .font: SearchResultAdClickedInfoBarUX.toastLabelFont,
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

    if let url = URL(string: SearchResultAdClickedInfoBarUX.learnMoreUrl) {
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
      $0.width.equalTo(SearchResultAdClickedInfoBarUX.toastCloseButtonWidth)
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
    self.tabManager.addTabAndSelect(
      URLRequest(url: URL(string: SearchResultAdClickedInfoBarUX.learnMoreUrl)!),
      isPrivate: false
    )
    dismiss(true)
    return false
  }

  @objc func buttonPressed(_ gestureRecognizer: UIGestureRecognizer) {
    dismiss(true)
  }
}
