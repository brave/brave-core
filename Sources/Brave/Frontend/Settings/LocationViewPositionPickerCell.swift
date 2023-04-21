// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Strings
import Preferences
import Static

/// Displays a picker that allows the user to pick the position of the location view (URL bar)
class LocationViewPositionPickerCell: UITableViewCell, Cell {
  private let stackView = UIStackView().then {
    $0.spacing = 12
    $0.distribution = .fillEqually
  }
  
  private let topBarButton = BarButton(
    image: UIImage(braveSystemNamed: "leo.browser.mobile-tabs-top")!,
    text: Strings.tabsOptionTopBar
  )
  
  private let bottomBarButton = BarButton(
    image: UIImage(braveSystemNamed: "leo.browser.mobile-tabs-bottom")!,
    text: Strings.tabsOptionBottomBar
  )
  
  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)
    
    contentView.addSubview(stackView)
    stackView.addArrangedSubview(topBarButton)
    stackView.addArrangedSubview(bottomBarButton)
    
    let isUsingBottomBar = Preferences.General.isUsingBottomBar.value
    topBarButton.isSelected = !isUsingBottomBar
    bottomBarButton.isSelected = isUsingBottomBar
    
    stackView.snp.makeConstraints {
      $0.leading.trailing.equalToSuperview().inset(24)
      $0.top.equalToSuperview().inset(12)
      $0.bottom.equalToSuperview().inset(16)
    }
    
    topBarButton.addTarget(self, action: #selector(tappedTopBarButton), for: .touchUpInside)
    bottomBarButton.addTarget(self, action: #selector(tappedBottomBarButton), for: .touchUpInside)
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  @objc private func tappedTopBarButton() {
    topBarButton.isSelected = true
    bottomBarButton.isSelected = false
    Preferences.General.isUsingBottomBar.value = false
  }
  
  @objc private func tappedBottomBarButton() {
    topBarButton.isSelected = false
    bottomBarButton.isSelected = true
    Preferences.General.isUsingBottomBar.value = true
  }
  
  class BarButton: UIControl {
    var image: UIImage
    var text: String
    
    private let checkmarkView = UIImageView().then {
      $0.preferredSymbolConfiguration = .init(font: .preferredFont(forTextStyle: .title3))
    }
    
    init(image: UIImage, text: String) {
      self.image = image
      self.text = text
      
      super.init(frame: .zero)
      
      let stackView = UIStackView()
      stackView.isUserInteractionEnabled = false
      stackView.axis = .vertical
      stackView.alignment = .center
      stackView.spacing = 2
      
      let imageView = UIImageView(image: image)
      imageView.preferredSymbolConfiguration = .init(font: .systemFont(ofSize: 40))
      let label = UILabel()
      label.text = text
      label.font = .preferredFont(forTextStyle: .callout)
      
      addSubview(stackView)
      stackView.addStackViewItems(
        .view(imageView),
        .view(label),
        .customSpace(8),
        .view(checkmarkView)
      )
      
      stackView.snp.makeConstraints {
        $0.edges.equalToSuperview()
      }
    }
    
    override var isSelected: Bool {
      didSet {
        checkmarkView.image = UIImage(braveSystemNamed: isSelected ? "leo.check.circle-outline" : "leo.radio.unchecked")
        checkmarkView.tintColor = isSelected ? .braveBlurpleTint : .braveDisabled
      }
    }
    
    override var isHighlighted: Bool {
      didSet {
        if isHighlighted {
          self.alpha = 0.5
        } else {
          UIViewPropertyAnimator(duration: 0.1, curve: .linear) {
            self.alpha = 1
          }
          .startAnimation()
        }
      }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
  }
}
