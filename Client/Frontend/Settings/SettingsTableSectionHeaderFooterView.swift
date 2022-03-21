/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Shared
import BraveUI
import UIKit

private struct SettingsTableSectionHeaderFooterViewUX {
  static let titleHorizontalPadding: CGFloat = 15
  static let titleVerticalPadding: CGFloat = 6
  static let titleVerticalLongPadding: CGFloat = 20
}

class SettingsTableSectionHeaderFooterView: UITableViewHeaderFooterView, TableViewReusable {

  enum TitleAlignment {
    case top
    case bottom
  }

  var titleAlignment: TitleAlignment = .bottom {
    didSet {
      remakeTitleAlignmentConstraints()
    }
  }

  lazy var titleLabel: UILabel = {
    var headerLabel = UILabel()
    headerLabel.font = UIFont.systemFont(ofSize: 12.0, weight: UIFont.Weight.regular)
    headerLabel.numberOfLines = 0
    return headerLabel
  }()

  override init(reuseIdentifier: String?) {
    super.init(reuseIdentifier: reuseIdentifier)
    addSubview(titleLabel)

    setupInitialConstraints()
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func setupInitialConstraints() {
    remakeTitleAlignmentConstraints()
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    titleLabel.text = nil
    titleAlignment = .bottom
  }

  fileprivate func remakeTitleAlignmentConstraints() {
    switch titleAlignment {
    case .top:
      titleLabel.snp.remakeConstraints { make in
        make.left.right.equalTo(self).inset(SettingsTableSectionHeaderFooterViewUX.titleHorizontalPadding)
        make.top.equalTo(self).offset(SettingsTableSectionHeaderFooterViewUX.titleVerticalPadding)
        make.bottom.equalTo(self).offset(-SettingsTableSectionHeaderFooterViewUX.titleVerticalLongPadding)
      }
    case .bottom:
      titleLabel.snp.remakeConstraints { make in
        make.left.right.equalTo(self).inset(SettingsTableSectionHeaderFooterViewUX.titleHorizontalPadding)
        make.bottom.equalTo(self).offset(-SettingsTableSectionHeaderFooterViewUX.titleVerticalPadding)
        make.top.equalTo(self).offset(SettingsTableSectionHeaderFooterViewUX.titleVerticalLongPadding)
      }
    }
  }
}
