// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

class IndentedImageTableViewCell: UITableViewCell {

  private let mainStackView = UIStackView().then {
    $0.spacing = 8
    $0.alignment = .fill
  }

  private let folderNameStackView = UIStackView().then {
    $0.axis = .vertical
    $0.distribution = .equalSpacing
  }

  let customImage = UIImageView().then {
    $0.image = UIImage(named: "shields-menu-icon", in: .module, compatibleWith: nil)!
    $0.tintColor = .braveLabel
    $0.contentMode = .scaleAspectFit
    $0.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)
    $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
  }

  let folderName = UILabel().then {
    $0.textAlignment = .left
    $0.textColor = .braveLabel
  }

  convenience init(image: UIImage) {
    self.init(style: .default, reuseIdentifier: nil)

    customImage.image = image
  }

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)

    indentationWidth = 20
    mainStackView.addArrangedSubview(customImage)

    let transparentLine = UIView.separatorLine
    transparentLine.backgroundColor = .clear

    [transparentLine, folderName, UIView.separatorLine].forEach(folderNameStackView.addArrangedSubview)

    mainStackView.addArrangedSubview(folderNameStackView)

    // Hide UITableViewCells separator, a custom one will be used.
    // This separator inset was problematic to update based on indentation.
    separatorInset = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: .greatestFiniteMagnitude)

    addSubview(mainStackView)

    mainStackView.snp.makeConstraints {
      $0.top.bottom.equalTo(self)
      $0.leading.trailing.equalTo(self).inset(8)
      $0.centerY.equalTo(self)
    }
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    let indentation = (CGFloat(indentationLevel) * indentationWidth)

    mainStackView.snp.remakeConstraints {
      $0.leading.equalTo(self).inset(indentation + 8)
      $0.top.bottom.equalTo(self)
      $0.trailing.equalTo(self).inset(8)
      $0.centerY.equalTo(self)
    }
  }
}
