/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

struct TwoLineCellUX {
  static let imageSize: CGFloat = 29
  static let imageCornerRadius: CGFloat = 8
  static let borderViewMargin: CGFloat = 16
  static let badgeSize: CGFloat = 16
  static let borderFrameSize: CGFloat = 32
  static let detailTextTopMargin: CGFloat = 0
}

class TwoLineTableViewCell: UITableViewCell, TableViewReusable {
  fileprivate let twoLineHelper = TwoLineCellHelper()

  let _textLabel = UILabel()
  let _detailTextLabel = UILabel()

  // Override the default labels with our own to disable default UITableViewCell label behaviours like dynamic type
  override var textLabel: UILabel? {
    return _textLabel
  }

  override var detailTextLabel: UILabel? {
    return _detailTextLabel
  }

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .subtitle, reuseIdentifier: reuseIdentifier)

    contentView.addSubview(_textLabel)
    contentView.addSubview(_detailTextLabel)

    twoLineHelper.setUpViews(contentView, textLabel: textLabel!, detailTextLabel: detailTextLabel!, imageView: imageView!)

    indentationWidth = 0
    layoutMargins = .zero

    separatorInset = UIEdgeInsets(top: 0, left: TwoLineCellUX.imageSize + 2 * TwoLineCellUX.borderViewMargin, bottom: 0, right: 0)
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    twoLineHelper.layoutSubviews(accessoryWidth: self.contentView.frame.origin.x)
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    self.textLabel!.alpha = 1
    self.imageView!.alpha = 1
    self.selectionStyle = .default
    separatorInset = UIEdgeInsets(top: 0, left: TwoLineCellUX.imageSize + 2 * TwoLineCellUX.borderViewMargin, bottom: 0, right: 0)
    twoLineHelper.setupDynamicFonts()
  }

  // Save background color on UITableViewCell "select" because it disappears in the default behavior
  override func setHighlighted(_ highlighted: Bool, animated: Bool) {
    let color = imageView?.backgroundColor
    super.setHighlighted(highlighted, animated: animated)
    imageView?.backgroundColor = color
  }

  // Save background color on UITableViewCell "select" because it disappears in the default behavior
  override func setSelected(_ selected: Bool, animated: Bool) {
    let color = imageView?.backgroundColor
    super.setSelected(selected, animated: animated)
    imageView?.backgroundColor = color
  }

  func setRightBadge(_ badge: UIImage?) {
    if let badge = badge {
      self.accessoryView = UIImageView(image: badge)
    } else {
      self.accessoryView = nil
    }
    twoLineHelper.hasRightBadge = badge != nil
  }

  func setLines(_ text: String?, detailText: String?, detailAttributedText: NSAttributedString? = nil) {
    twoLineHelper.setLines(text, detailText: detailText, detailAttributedText: detailAttributedText)
  }

  func mergeAccessibilityLabels(_ views: [AnyObject?]? = nil) {
    twoLineHelper.mergeAccessibilityLabels(views)
  }
}

class SiteTableViewCell: TwoLineTableViewCell {
  let borderView = UIView()

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .subtitle, reuseIdentifier: reuseIdentifier)
    backgroundColor = .secondaryBraveBackground

    twoLineHelper.setUpViews(contentView, textLabel: textLabel!, detailTextLabel: detailTextLabel!, imageView: imageView!)
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    twoLineHelper.layoutSubviews(accessoryWidth: self.contentView.frame.origin.x)
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}

class BookmarkTableViewCell: TwoLineTableViewCell {
  /// Holds url domain for bookmarks or folder name for folders.
  /// This property is used to see if the cell's image should be reused.
  var domainOrFolderName: String?

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .subtitle, reuseIdentifier: reuseIdentifier)
    guard let textLabel = textLabel, let detailTextLabel = detailTextLabel,
      let imageView = imageView
    else { return }

    backgroundColor = .secondaryBraveBackground

    twoLineHelper.setUpViews(
      contentView,
      textLabel: textLabel,
      detailTextLabel: detailTextLabel,
      imageView: imageView)
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    twoLineHelper.layoutSubviews(accessoryWidth: self.contentView.frame.origin.x)
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}

class TwoLineHeaderFooterView: UITableViewHeaderFooterView {
  fileprivate let twoLineHelper = TwoLineCellHelper()

  // UITableViewHeaderFooterView includes textLabel and detailTextLabel, so we can't override
  // them.  Unfortunately, they're also used in ways that interfere with us just using them: I get
  // hard crashes in layout if I just use them; it seems there's a battle over adding to the
  // contentView.  So we add our own members, and cover up the other ones.
  let _textLabel = UILabel()
  let _detailTextLabel = UILabel()

  let imageView = UIImageView()

  // Yes, this is strange.
  override var textLabel: UILabel? {
    return _textLabel
  }

  // Yes, this is strange.
  override var detailTextLabel: UILabel? {
    return _detailTextLabel
  }

  override init(reuseIdentifier: String?) {
    super.init(reuseIdentifier: reuseIdentifier)
    twoLineHelper.setUpViews(contentView, textLabel: _textLabel, detailTextLabel: _detailTextLabel, imageView: imageView)

    contentView.addSubview(_textLabel)
    contentView.addSubview(_detailTextLabel)
    contentView.addSubview(imageView)

    layoutMargins = .zero
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    twoLineHelper.layoutSubviews()
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    twoLineHelper.setupDynamicFonts()
  }

  func mergeAccessibilityLabels(_ views: [AnyObject?]? = nil) {
    twoLineHelper.mergeAccessibilityLabels(views)
  }
}

private class TwoLineCellHelper {
  weak var container: UIView?
  var textLabel: UILabel!
  var detailTextLabel: UILabel!
  var imageView: UIImageView!
  var hasRightBadge: Bool = false

  // TODO: Not ideal. We should figure out a better way to get this initialized.
  func setUpViews(_ container: UIView, textLabel: UILabel, detailTextLabel: UILabel, imageView: UIImageView) {
    self.container = container
    self.textLabel = textLabel
    self.detailTextLabel = detailTextLabel
    self.imageView = imageView

    if let headerView = self.container as? UITableViewHeaderFooterView {
      headerView.contentView.backgroundColor = .clear
    } else {
      self.container?.backgroundColor = .clear
    }

    textLabel.textColor = .braveLabel
    detailTextLabel.textColor = .secondaryBraveLabel
    setupDynamicFonts()

    imageView.contentMode = .scaleAspectFill
    imageView.layer.cornerRadius = 6  // hmm
    imageView.layer.cornerCurve = .continuous
    imageView.layer.masksToBounds = true
  }

  func setupDynamicFonts() {
    textLabel.font = DynamicFontHelper.defaultHelper.DeviceFontHistoryPanel
    detailTextLabel.font = DynamicFontHelper.defaultHelper.SmallSizeRegularWeightAS
  }

  func layoutSubviews(accessoryWidth: CGFloat = 0) {
    guard let container = self.container else {
      return
    }
    let height = container.frame.height
    let textLeft = TwoLineCellUX.imageSize + 2 * TwoLineCellUX.borderViewMargin
    let textLabelHeight = textLabel.intrinsicContentSize.height
    let detailTextLabelHeight = detailTextLabel.intrinsicContentSize.height
    var contentHeight = textLabelHeight
    if detailTextLabelHeight > 0 {
      contentHeight += detailTextLabelHeight + TwoLineCellUX.detailTextTopMargin
    }

    let textRightInset: CGFloat = hasRightBadge ? TwoLineCellUX.badgeSize : 0

    textLabel.frame = CGRect(
      x: textLeft, y: (height - contentHeight) / 2,
      width: container.frame.width - textLeft - TwoLineCellUX.borderViewMargin - textRightInset, height: textLabelHeight)
    detailTextLabel.frame = CGRect(
      x: textLeft, y: textLabel.frame.maxY + TwoLineCellUX.detailTextTopMargin,
      width: container.frame.width - textLeft - TwoLineCellUX.borderViewMargin - textRightInset, height: detailTextLabelHeight)

    // Like the comment above, this is not ideal. This code should probably be refactored to use autolayout. That will remove a lot of the pixel math and remove code duplication.

    if UIApplication.shared.userInterfaceLayoutDirection == .leftToRight {
      imageView.frame = CGRect(x: TwoLineCellUX.borderViewMargin, y: (height - TwoLineCellUX.imageSize) / 2, width: TwoLineCellUX.imageSize, height: TwoLineCellUX.imageSize)
    } else {
      imageView.frame = CGRect(x: container.frame.width - TwoLineCellUX.imageSize - TwoLineCellUX.borderViewMargin, y: (height - TwoLineCellUX.imageSize) / 2, width: TwoLineCellUX.imageSize, height: TwoLineCellUX.imageSize)

      textLabel.frame = textLabel.frame.offsetBy(dx: -(TwoLineCellUX.imageSize + TwoLineCellUX.borderViewMargin - textRightInset), dy: 0)
      detailTextLabel.frame = detailTextLabel.frame.offsetBy(dx: -(TwoLineCellUX.imageSize + TwoLineCellUX.borderViewMargin - textRightInset), dy: 0)

      // If the cell has an accessory, shift them all to the left even more. Only required on RTL.
      if accessoryWidth != 0 {
        imageView.frame = imageView.frame.offsetBy(dx: -accessoryWidth, dy: 0)
        textLabel.frame = textLabel.frame.offsetBy(dx: -accessoryWidth, dy: 0)
        detailTextLabel.frame = detailTextLabel.frame.offsetBy(dx: -accessoryWidth, dy: 0)
      }
    }
  }

  func setLines(_ text: String?, detailText: String?, detailAttributedText: NSAttributedString? = nil) {
    if text?.isEmpty ?? true {
      textLabel.text = detailText
      detailTextLabel.text = nil
    } else {
      textLabel.text = text
      if let attributedText = detailAttributedText {
        detailTextLabel.attributedText = attributedText
      } else {
        detailTextLabel.text = detailText
      }
    }
  }

  func mergeAccessibilityLabels(_ labels: [AnyObject?]?) {
    let labels = labels ?? [textLabel, imageView, detailTextLabel]

    let label = labels.map({ (label: AnyObject?) -> NSAttributedString? in
      var label = label
      if let view = label as? UIView {
        label = view.value(forKey: "accessibilityLabel") as (AnyObject?)
      }

      if let attrString = label as? NSAttributedString {
        return attrString
      } else if let string = label as? String {
        return NSAttributedString(string: string)
      } else {
        return nil
      }
    }).filter({
      $0 != nil
    }).reduce(NSMutableAttributedString(string: ""), {
      if $0.length > 0 {
        $0.append(NSAttributedString(string: ", "))
      }
      $0.append($1!)
      return $0
    })

    container?.isAccessibilityElement = true
    container?.setValue(NSAttributedString(attributedString: label), forKey: "accessibilityLabel")
  }
}
