/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

/// A table cell subclass which rids the default textLabel/detailTextLabel so that they may be
/// properly laid out via AutoLayout
///
/// - note: Currently only supports `.default` and `.value1` typed table cells
class TableViewCell: UITableViewCell, TableViewReusable {
  enum ManualSeparator {
    case top
    case bottom
  }
  /// Manual separators to display
  var manualSeparators: [ManualSeparator] = [.bottom]
  /// The color for the manual separators
  var separatorColor = UIColor(white: 0.8, alpha: 1.0)
  /// The height of the manual separators
  var separatorHeight: CGFloat = 1.0 / UIScreen.main.scale
  
  /// The replacement for `textLabel`
  let label: UILabel
  /// The replacement for `detailTextLabel`
  let accessoryLabel: UILabel?
  
  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    label = UILabel().then {
      $0.numberOfLines = 0
    }
    
    switch style {
    case .default, .subtitle, .value2:
      accessoryLabel = nil
    case .value1:
      accessoryLabel = UILabel().then {
        $0.setContentCompressionResistancePriority(.required, for: .horizontal)
        $0.appearanceTextColor = Colors.grey800
      }
    @unknown default:
      fatalError("Currently not supported")
    }
    
    super.init(style: style, reuseIdentifier: reuseIdentifier)
    
    label.appearanceTextColor = .black
    backgroundColor = .white
    
    contentView.addSubview(label)
    if let accessoryLabel = accessoryLabel {
      contentView.addSubview(accessoryLabel)
    }
    
    setNeedsUpdateConstraints()
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override func prepareForReuse() {
    super.prepareForReuse()
    
    label.text = nil
    accessoryLabel?.text = nil
    // Not sure if this is done by UITableViewCell:
    imageView?.image = nil
    accessoryView = nil
    
    setNeedsDisplay()
  }
  
  override func draw(_ rect: CGRect) {
    super.draw(rect)
    
    UIColor(white: 0.8, alpha: 1.0).setFill()
    if manualSeparators.contains(.top) {
      UIRectFill(CGRect(x: 0, y: 0, width: rect.width, height: separatorHeight))
    }
    if manualSeparators.contains(.bottom) {
      UIRectFill(CGRect(x: 0, y: rect.maxY - separatorHeight, width: rect.width, height: separatorHeight))
    }
  }
  
  override func updateConstraints() {
    super.updateConstraints()
    
    label.snp.remakeConstraints {
      $0.top.equalToSuperview().inset(layoutMargins.top)
      if let imageView = imageView, imageView.image != nil, imageView.superview != nil {
        $0.leading.equalTo(imageView.snp.trailing).offset(layoutMargins.left)
      } else {
        $0.leading.equalToSuperview().inset(layoutMargins.left)
      }
      if let accessoryLabel = accessoryLabel {
        $0.trailing.lessThanOrEqualTo(accessoryLabel.snp.leading).offset(-layoutMargins.right)
      } else {
        $0.trailing.equalToSuperview().inset(layoutMargins.right)
      }
      $0.bottom.equalToSuperview().inset(layoutMargins.bottom)
    }
    
    if let accessoryLabel = accessoryLabel {
      accessoryLabel.snp.remakeConstraints {
        if accessoryType == .none {
          $0.trailing.equalToSuperview().inset(layoutMargins.right)
        } else {
          $0.trailing.equalToSuperview().inset(6)
        }
        $0.centerY.equalToSuperview()
      }
    }
  }
  
  override func systemLayoutSizeFitting(_ targetSize: CGSize, withHorizontalFittingPriority horizontalFittingPriority: UILayoutPriority, verticalFittingPriority: UILayoutPriority) -> CGSize {
    var size = super.systemLayoutSizeFitting(targetSize, withHorizontalFittingPriority: horizontalFittingPriority, verticalFittingPriority: verticalFittingPriority)
    let hasSwitchAccessoryView = accessoryView is UISwitch
    size.height = max(hasSwitchAccessoryView ? 47.0 : 44.0, size.height)
    return size
  }
  
  // MARK: - Unavailable
  
  // swiftlint:disable unused_setter_value
  @available(*, unavailable)
  override var textLabel: UILabel? {
    get { return nil }
    set { }
  }
  
  @available(*, unavailable)
  override var detailTextLabel: UILabel? {
    get { return nil }
    set { }
  }
  // swiftlint:enable unused_setter_value
}

/// Just a TableViewCell which uses the `value1` style by default
class Value1TableViewCell: TableViewCell {
  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .value1, reuseIdentifier: reuseIdentifier)
  }
}
