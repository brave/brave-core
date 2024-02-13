/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Preferences
import Combine

class TabBarCell: UICollectionViewCell {

  lazy var titleLabel: UILabel = {
    let label = UILabel()
    label.textAlignment = .center
    return label
  }()

  private lazy var closeButton: UIButton = {
    let button = UIButton()
    button.addTarget(self, action: #selector(closeTab), for: .touchUpInside)
    button.setImage(UIImage(named: "close_tab_bar", in: .module, compatibleWith: nil)!.template, for: .normal)
    button.tintColor = .braveLabel
    // Close button is a bit wider to increase tap area, this aligns the 'X' image closer to the right.
    button.imageEdgeInsets.left = 6
    return button
  }()
  
  private let highlightView = UIView().then {
    $0.layer.cornerRadius = 4
    $0.layer.cornerCurve = .continuous
    $0.layer.shadowOffset = CGSize(width: 0, height: 1)
    $0.layer.shadowRadius = 2
  }
  
  private let separatorLine = UIView().then {
    $0.isHidden = true
  }

  var currentIndex: Int = -1 {
    didSet {
      isSelected = currentIndex == tabManager?.currentDisplayedIndex
      separatorLine.isHidden = isSelected || 
        currentIndex == 0 ||
        currentIndex == (tabManager?.currentDisplayedIndex ?? 0) + 1
    }
  }
  weak var tab: Tab?
  weak var tabManager: TabManager? {
    didSet {
      updateColors()
      privateModeCancellable = tabManager?.privateBrowsingManager
        .$isPrivateBrowsing
        .removeDuplicates()
        .receive(on: RunLoop.main)
        .sink(receiveValue: { [weak self] _ in
          self?.updateColors()
        })
    }
  }

  var closeTabCallback: ((Tab) -> Void)?
  private var cancellables: Set<AnyCancellable> = []

  override init(frame: CGRect) {
    super.init(frame: frame)
    
    [highlightView, closeButton, titleLabel, separatorLine].forEach { contentView.addSubview($0) }
    initConstraints()
    updateFont()
    
    isSelected = false
  }

  private var privateModeCancellable: AnyCancellable?
  private func updateColors() {
    let browserColors: any BrowserColors = tabManager?.privateBrowsingManager.browserColors ?? .standard
    backgroundColor = browserColors.tabBarTabBackground
    separatorLine.backgroundColor = browserColors.dividerSubtle
    highlightView.backgroundColor = isSelected ? browserColors.tabBarTabActiveBackground : .clear
    highlightView.layer.shadowOpacity = isSelected ? 0.05 : 0.0
    highlightView.layer.shadowColor = UIColor.black.cgColor
    closeButton.tintColor = browserColors.iconDefault
    titleLabel.textColor = isSelected ? browserColors.textPrimary : browserColors.textSecondary
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func initConstraints() {
    highlightView.snp.makeConstraints {
      $0.edges.equalToSuperview().inset(2)
    }
    
    titleLabel.snp.makeConstraints { make in
      make.top.bottom.equalTo(self)
      make.left.equalTo(self).inset(16)
      make.right.equalTo(closeButton.snp.left)
    }

    closeButton.snp.makeConstraints { make in
      make.top.bottom.equalTo(self)
      make.right.equalTo(self).inset(2)
      make.width.equalTo(30)
    }
    
    separatorLine.snp.makeConstraints { make in
      make.left.equalToSuperview()
      make.width.equalTo(1)
      make.top.bottom.equalToSuperview().inset(5)
    }
  }

  override var isSelected: Bool {
    didSet {
      configure()
    }
  }

  func configure() {
    if isSelected {
      titleLabel.alpha = 1.0
      closeButton.isHidden = false
    }
    // Prevent swipe and release outside- deselects cell.
    else if currentIndex != tabManager?.currentDisplayedIndex {
      titleLabel.alpha = 0.8
      closeButton.isHidden = true
    }
    updateFont()
    updateColors()
  }
  
  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateFont()
  }
  
  private func updateFont() {
    let clampedTraitCollection = self.traitCollection.clampingSizeCategory(maximum: .extraExtraLarge)
    let font = UIFont.preferredFont(forTextStyle: .caption1, compatibleWith: clampedTraitCollection)
    titleLabel.font = .systemFont(ofSize: font.pointSize, weight: isSelected ? .semibold : .regular)
  }

  @objc func closeTab() {
    guard let tab = tab else { return }
    closeTabCallback?(tab)
  }

  fileprivate var titleUpdateScheduled = false
  func updateTitleThrottled(for tab: Tab) {
    if titleUpdateScheduled {
      return
    }
    titleUpdateScheduled = true
    DispatchQueue.main.asyncAfter(deadline: .now() + 0.2) { [weak self] in
      guard let strongSelf = self else { return }
      strongSelf.titleUpdateScheduled = false
      strongSelf.titleLabel.text = tab.displayTitle
    }
  }
  
  override func layoutSubviews() {
    super.layoutSubviews()
    
    highlightView.layer.shadowPath = UIBezierPath(roundedRect: highlightView.bounds, cornerRadius: 4).cgPath
  }
}
