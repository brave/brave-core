/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SnapKit
import Shared
import UIKit

private struct TabsButtonUX {
  static let cornerRadius: CGFloat = 3
  static let borderStrokeWidth: CGFloat = 1.5
}

class TabsButton: UIButton {

  private let countLabel = UILabel().then {
    $0.textAlignment = .center
    $0.isUserInteractionEnabled = false
  }

  private let borderView = UIView().then {
    $0.layer.borderWidth = TabsButtonUX.borderStrokeWidth
    $0.layer.cornerRadius = TabsButtonUX.cornerRadius
    $0.layer.cornerCurve = .continuous
    $0.isUserInteractionEnabled = false
  }
  
  var browserColors: any BrowserColors = .standard {
    didSet {
      updateForTraitCollectionAndBrowserColors()
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    accessibilityTraits.insert(.button)
    isAccessibilityElement = true
    accessibilityLabel = Strings.showTabs

    addSubview(borderView)
    addSubview(countLabel)

    countLabel.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    updateForTraitCollectionAndBrowserColors()
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override var isHighlighted: Bool {
    didSet {
      let color: UIColor = isHighlighted ? browserColors.iconActive : browserColors.iconDefault
      countLabel.textColor = color
      borderView.layer.borderColor = color.resolvedColor(with: traitCollection).cgColor
    }
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateForTraitCollectionAndBrowserColors()
  }
  
  private func updateForTraitCollectionAndBrowserColors() {
    // CGColor's do not get automatic updates
    countLabel.textColor = isHighlighted ? browserColors.iconActive : browserColors.iconDefault
    borderView.layer.borderColor = isHighlighted ? browserColors.iconActive.cgColor : browserColors.iconDefault.resolvedColor(with: traitCollection).cgColor
    
    let toolbarTraitCollection = UITraitCollection(preferredContentSizeCategory: traitCollection.toolbarButtonContentSizeCategory)
    let metrics = UIFontMetrics(forTextStyle: .body)
    borderView.snp.remakeConstraints {
      $0.center.equalToSuperview()
      $0.size.equalTo(metrics.scaledValue(for: 20, compatibleWith: toolbarTraitCollection))
    }
    countLabel.font = .systemFont(ofSize: UIFont.preferredFont(forTextStyle: .caption2, compatibleWith: toolbarTraitCollection).pointSize, weight: .bold)
  }

  private var currentCount: Int?

  func updateTabCount(_ count: Int) {
    let count = max(count, 1)
    // Sometimes tabs count state is held in the cloned tabs button.
    let infinity = "\u{221E}"
    let countToBe = (count < 100) ? "\(count)" : infinity
    currentCount = count
    self.countLabel.text = countToBe
    self.accessibilityValue = countToBe
  }

  override func contextMenuInteraction(_ interaction: UIContextMenuInteraction, willDisplayMenuFor configuration: UIContextMenuConfiguration, animator: UIContextMenuInteractionAnimating?) {
    UIImpactFeedbackGenerator(style: .medium).bzzt()
  }
}
