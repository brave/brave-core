// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import SnapKit
import Preferences
import Combine

class HeaderContainerView: UIView {
  
  let expandedBarStackView = UIStackView().then {
    $0.axis = .vertical
  }
  let collapsedBarContainerView = UIControl().then {
    $0.alpha = 0
  }
  var isUsingBottomBar: Bool = false {
    didSet {
      updateConstraints()
    }
  }
  let line = UIView()
  
  /// Container view for both the expanded & collapsed variants of the bar
  let contentView = UIView()
  
  private var cancellables: Set<AnyCancellable> = []
  private let privateBrowsingManager: PrivateBrowsingManager
  
  init(privateBrowsingManager: PrivateBrowsingManager) {
    self.privateBrowsingManager = privateBrowsingManager
    
    super.init(frame: .zero)
    
    addSubview(contentView)
    contentView.addSubview(expandedBarStackView)
    contentView.addSubview(collapsedBarContainerView)
    addSubview(line)
    
    contentView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    collapsedBarContainerView.snp.makeConstraints {
      $0.leading.trailing.equalTo(safeAreaLayoutGuide)
      $0.bottom.equalToSuperview()
    }
    expandedBarStackView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    
    updateColors()
    
    privateBrowsingManager
      .$isPrivateBrowsing
      .removeDuplicates()
      .receive(on: RunLoop.main)
      .sink(receiveValue: { [weak self] _ in
        self?.updateColors()
      })
      .store(in: &cancellables)
  }
  
  override func updateConstraints() {
    super.updateConstraints()
    
    collapsedBarContainerView.snp.remakeConstraints {
      if isUsingBottomBar {
        $0.top.equalToSuperview()
      } else {
        $0.bottom.equalToSuperview()
      }
      $0.leading.trailing.equalTo(safeAreaLayoutGuide)
    }
    line.snp.remakeConstraints {
      if self.isUsingBottomBar {
        $0.bottom.equalTo(self.snp.top)
      } else {
        $0.top.equalTo(self.snp.bottom)
      }
      $0.leading.trailing.equalToSuperview()
      $0.height.equalTo(1.0 / UIScreen.main.scale)
    }
  }
  
  private func updateColors() {
    let browserColors = privateBrowsingManager.browserColors
    line.backgroundColor = browserColors.dividerSubtle
    backgroundColor = browserColors.chromeBackground
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
