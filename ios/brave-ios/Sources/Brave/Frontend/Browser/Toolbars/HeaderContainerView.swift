// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Combine
import Foundation
import Preferences
import SnapKit
import UIKit

class HeaderContainerView: UIView {

  let expandedBarStackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 8
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

  /// The row of `expandedBarStackView` that holds `barContentView`.
  ///
  /// When using a top bar only this row is inset horizontally to avoid corner intrusions (such as
  /// the status bar on an unfolded iPhone Duo or window controls on iPad), while other rows such as
  /// the tabs bar span the full width of the header below it.
  private let barContainerView = UIView()

  /// The main contents of the expanded bar (such as the URL bar)
  var barContentView: UIView? {
    didSet {
      oldValue?.removeFromSuperview()
      if let barContentView {
        barContainerView.addSubview(barContentView)
      }
      setNeedsUpdateConstraints()
    }
  }

  private var cancellables: Set<AnyCancellable> = []
  private let privateBrowsingManager: PrivateBrowsingManager

  init(privateBrowsingManager: PrivateBrowsingManager) {
    self.privateBrowsingManager = privateBrowsingManager

    super.init(frame: .zero)

    addSubview(contentView)
    contentView.addSubview(expandedBarStackView)
    contentView.addSubview(collapsedBarContainerView)
    expandedBarStackView.addArrangedSubview(barContainerView)
    addSubview(line)

    contentView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    collapsedBarContainerView.snp.makeConstraints {
      $0.leading.trailing.equalTo(horizontalSafeAreaLayoutGuide)
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

  /// The insets applied to the bar's contents when using a top bar
  private(set) var contentInsets: NSDirectionalEdgeInsets = .zero {
    didSet {
      if oldValue != contentInsets {
        setNeedsUpdateConstraints()
      }
    }
  }

  /// Updates the insets applied to the bar's contents from the header's current safe area and
  /// layout regions.
  ///
  /// This should only be called while the header is fully expanded. While collapsing, the header is
  /// moved above the top edge of the screen which grows its safe area and layout region insets by
  /// the same distance, which would otherwise keep the contents in place instead of moving with it.
  func updateContentInsets() {
    contentInsets = topBarContentInsets
  }

  /// The safe area used to inset the bar's contents horizontally, adapting to corner intrusions
  /// such as a status bar that only occupies a top corner of the display
  private var horizontalSafeAreaLayoutGuide: UILayoutGuide {
    if #available(iOS 26.0, *) {
      return layoutGuide(for: .safeArea(cornerAdaptation: .horizontal))
    }
    return safeAreaLayoutGuide
  }

  override func updateConstraints() {
    super.updateConstraints()

    contentView.snp.remakeConstraints {
      if isUsingBottomBar {
        $0.edges.equalToSuperview()
      } else {
        // Only the bar's contents are inset horizontally, see `barContainerView`
        $0.top.equalToSuperview().inset(contentInsets.top)
        $0.leading.trailing.bottom.equalToSuperview()
      }
    }
    barContentView?.snp.remakeConstraints {
      $0.top.bottom.equalToSuperview()
      if isUsingBottomBar {
        $0.leading.trailing.equalTo(horizontalSafeAreaLayoutGuide)
      } else {
        $0.leading.equalToSuperview().inset(contentInsets.leading)
        $0.trailing.equalToSuperview().inset(contentInsets.trailing)
      }
    }
    collapsedBarContainerView.snp.remakeConstraints {
      if isUsingBottomBar {
        $0.top.equalToSuperview()
      } else {
        $0.bottom.equalToSuperview()
      }
      // Center the collapsed bar on the header itself rather than the safe area so that it remains
      // centered on screen, while keeping it within the safe area
      $0.centerX.equalTo(self)
      if isUsingBottomBar {
        $0.leading.greaterThanOrEqualTo(horizontalSafeAreaLayoutGuide)
        $0.trailing.lessThanOrEqualTo(horizontalSafeAreaLayoutGuide)
      } else {
        $0.leading.greaterThanOrEqualToSuperview().inset(contentInsets.leading)
        $0.trailing.lessThanOrEqualToSuperview().inset(contentInsets.trailing)
      }
      $0.width.equalTo(self).priority(.high)
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
