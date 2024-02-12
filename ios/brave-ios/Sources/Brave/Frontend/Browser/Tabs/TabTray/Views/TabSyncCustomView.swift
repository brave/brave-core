// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI
import BraveStrings

protocol TabSyncHeaderViewDelegate: AnyObject {
  func toggleSection(_ header: TabSyncHeaderView, section: Int)
  func hideForNow(_ header: TabSyncHeaderView, section: Int)
  func openAll(_ header: TabSyncHeaderView, section: Int)
}

class TabSyncHeaderView: UITableViewHeaderFooterView, TableViewReusable {
    
  struct UX {
    static let labelOffset = 11.0
    static let imageSize = 24.0
    static let iconSize = 20.0
  }
  
  weak var delegate: TabSyncHeaderViewDelegate?
  var section = 0
  var isCollapsed = false {
    didSet {
      if oldValue == isCollapsed { return }
      
      let rotationAngle = isCollapsed ? -1 * (.pi / 2) : 0.0
      arrowIconView.transform = CGAffineTransform(rotationAngle: rotationAngle)
    }
  }
  
  let imageIconView = UIImageView().then {
    $0.contentMode = .scaleAspectFit
    $0.tintColor = .braveLabel
  }
    
  let labelStackView = UIStackView().then {
    $0.axis = .vertical
    $0.alignment = .leading
    $0.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
  }
  
  let titleLabel = UILabel().then {
    $0.textColor = .braveLabel
    $0.font = .preferredFont(for: .body, weight: .bold)
  }
  
  let descriptionLabel = UILabel().then {
    $0.textColor = .secondaryBraveLabel
    $0.font = .preferredFont(forTextStyle: .footnote)
  }
  
  let arrowIconView = UIImageView().then {
    $0.image = UIImage(systemName: "chevron.down")
    $0.contentMode = .scaleAspectFit
    $0.tintColor = .braveLabel
    $0.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)
  }
    
  override init(reuseIdentifier: String?) {
    super.init(reuseIdentifier: reuseIdentifier)
    contentView.backgroundColor = .clear
    
    contentView.addSubview(imageIconView)
    contentView.addSubview(labelStackView)
    contentView.addSubview(arrowIconView)

    labelStackView.addArrangedSubview(titleLabel)
    labelStackView.setCustomSpacing(3.0, after: titleLabel)
    labelStackView.addArrangedSubview(descriptionLabel)

    imageIconView.snp.makeConstraints {
      $0.leading.equalToSuperview().inset(UX.iconSize)
      $0.centerY.equalToSuperview()
      $0.size.equalTo(UX.imageSize)
    }
    
    labelStackView.snp.makeConstraints {
      $0.leading.equalTo(imageIconView.snp.trailing).offset(UX.iconSize)
      $0.trailing.equalToSuperview().offset(-UX.labelOffset)
      $0.top.equalToSuperview().offset(UX.labelOffset)
      $0.bottom.equalToSuperview().offset(-UX.labelOffset)
    }
    
    arrowIconView.snp.makeConstraints {
      $0.trailing.equalToSuperview().inset(UX.labelOffset)
      $0.leading.greaterThanOrEqualTo(titleLabel.snp.trailing).inset(-TwoLineCellUX.borderViewMargin)
      $0.centerY.equalToSuperview()
      $0.size.equalTo(UX.iconSize)
    }

    addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(tapHeader(_:))))
    
    let toolBarInteraction = UIContextMenuInteraction(delegate: self)
    self.contentView.addInteraction(toolBarInteraction)
  }
    
  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  /// Tap action where arrow indicating header status rotates with animation
  /// - Parameter gestureRecognizer: Tap GestureRecognizer
  @objc func tapHeader(_ gestureRecognizer: UITapGestureRecognizer) {
    guard let cell = gestureRecognizer.view as? TabSyncHeaderView else {
      return
    }
    
    setCollapsed { [weak self] in
      guard let self = self else { return }
      
      self.delegate?.toggleSection(self, section: cell.section)
    }
  }
    
  private func setCollapsed(completion: (() -> Void)? = nil) {
    CATransaction.begin()
    
    let animation = CABasicAnimation(keyPath: "transform.rotation").then {
      $0.toValue = isCollapsed ? 0.0 : -1 * (.pi / 2)
      $0.duration = 0.2
      $0.isRemovedOnCompletion = false
      $0.fillMode = CAMediaTimingFillMode.forwards
    }

    CATransaction.setCompletionBlock { [weak self] in
      guard let self = self else { return }
      self.isCollapsed = !self.isCollapsed
      completion?()
    }
    
    arrowIconView.layer.add(animation, forKey: nil)
    
    CATransaction.commit()
  }
}

extension TabSyncHeaderView: UIContextMenuInteractionDelegate {
  public func contextMenuInteraction(_ interaction: UIContextMenuInteraction, configurationForMenuAtLocation location: CGPoint) -> UIContextMenuConfiguration? {
    return UIContextMenuConfiguration(identifier: nil, previewProvider: nil) { [unowned self] _ in
      var actionMenuChildren: [UIAction] = []

      let allOpenAction = UIAction(
        title: Strings.OpenTabs.openSessionOpenAllActionTitle,
        image: UIImage(systemName: "plus"),
        handler: UIAction.deferredActionHandler { _ in
          self.delegate?.openAll(self, section: self.section)
        })

      let hideForAction = UIAction(
        title: Strings.OpenTabs.openSessionHideAllActionTitle,
        image: UIImage(braveSystemNamed: "leo.eye.off"),
        attributes: .destructive,
        handler: UIAction.deferredActionHandler { _ in
          self.delegate?.hideForNow(self, section: self.section)
        })

      actionMenuChildren = [allOpenAction, hideForAction]

      return UIMenu(title: "", identifier: nil, children: actionMenuChildren)
    }
  }
  
  func contextMenuInteraction(_ interaction: UIContextMenuInteraction, configuration: UIContextMenuConfiguration, highlightPreviewForItemWithIdentifier identifier: NSCopying) -> UITargetedPreview? {
    let parameters = UIPreviewParameters().then {
      $0.backgroundColor = .clear
    }
  
    return UITargetedPreview(view: self, parameters: parameters)
  }
  
  func contextMenuInteraction(_ interaction: UIContextMenuInteraction, configuration: UIContextMenuConfiguration, dismissalPreviewForItemWithIdentifier identifier: NSCopying) -> UITargetedPreview? {
    self.contextMenuInteraction(interaction, configuration: configuration, highlightPreviewForItemWithIdentifier: identifier)
  }
}

class TabSyncTableViewCell: UITableViewCell, TableViewReusable {
    
  struct UX {
    static let labelOffset = 11.0
    static let imageSize = 32.0
  }
  
  weak var delegate: TabSyncHeaderViewDelegate?
  
  let imageIconView = UIImageView().then {
    $0.contentMode = .scaleAspectFit
    $0.tintColor = .braveLabel
  }
    
  let labelStackView = UIStackView().then {
    $0.axis = .vertical
    $0.alignment = .leading
    $0.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
  }
  
  let titleLabel = UILabel().then {
    $0.textColor = .braveLabel
    $0.font = .preferredFont(forTextStyle: .footnote)
  }
  
  let descriptionLabel = UILabel().then {
    $0.textColor = .secondaryBraveLabel
    $0.font = .preferredFont(forTextStyle: .subheadline)
  }
  
  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)

    contentView.backgroundColor = .sectionBackgroundColor
    
    contentView.addSubview(imageIconView)
    contentView.addSubview(labelStackView)

    labelStackView.addArrangedSubview(titleLabel)
    labelStackView.setCustomSpacing(3.0, after: titleLabel)
    labelStackView.addArrangedSubview(descriptionLabel)

    imageIconView.snp.makeConstraints {
      $0.leading.equalToSuperview().inset(TwoLineCellUX.borderViewMargin)
      $0.centerY.equalToSuperview()
      $0.size.equalTo(UX.imageSize)
    }
    
    labelStackView.snp.makeConstraints {
      $0.leading.equalTo(imageIconView.snp.trailing).offset(TwoLineCellUX.borderViewMargin)
      $0.trailing.equalToSuperview().offset(-UX.labelOffset)
      $0.top.equalToSuperview().offset(UX.labelOffset)
      $0.bottom.equalToSuperview().offset(-UX.labelOffset)
    }
  }
    
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  func setLines(_ text: String?, detailText: String?) {
    titleLabel.text = text
    descriptionLabel.text = detailText
  }
}

private extension UIColor {
  
  static var sectionBackgroundColor: UIColor {
    return UIColor { $0.userInterfaceStyle == .dark ? .black : .white }
  }
}
