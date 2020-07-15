// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI

class ExclusionListCell: UITableViewCell, TableViewReusable {
  
  private let siteStackView = UIStackView()
  
  let siteImageView = PublisherIconCircleImageView(size: 28, inset: 4)
  
  let verifiedStatusImageView = UIImageView(image: UIImage(frameworkResourceNamed: "icn-verify")).then {
    $0.isHidden = true
  }
  let siteNameLabel = UILabel().then {
    $0.appearanceTextColor = SettingsUX.bodyTextColor
    $0.font = SettingsUX.bodyFont
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.numberOfLines = 0
  }
  
  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: nil)
    
    siteStackView.spacing = verifiedStatusImageView.image?.size.width ?? 10.0
    
    backgroundColor = .white
    
    contentView.addSubview(siteStackView)
    contentView.addSubview(verifiedStatusImageView)
    siteStackView.addArrangedSubview(siteImageView)
    siteStackView.addArrangedSubview(siteNameLabel)
    
    siteStackView.snp.makeConstraints {
      $0.top.bottom.equalTo(contentView).inset(10.0)
      $0.leading.trailing.equalTo(contentView).inset(15.0)
    }
    verifiedStatusImageView.snp.makeConstraints {
      $0.top.equalTo(siteStackView).offset(-4.0)
      $0.leading.equalTo(siteImageView.snp.trailing).offset(-8.0)
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override func draw(_ rect: CGRect) {
    super.draw(rect)
    
    UIColor(white: 0.85, alpha: 1.0).setFill()
    let height = 1.0 / UIScreen.main.scale
    UIRectFill(CGRect(x: 0, y: rect.maxY - height, width: rect.width, height: height))
  }
  
  // MARK: - Unavailable
  
  // swiftlint:disable unused_setter_value
  @available(*, unavailable)
  override var textLabel: UILabel? {
    get { return super.textLabel }
    set { }
  }
  
  @available(*, unavailable)
  override var detailTextLabel: UILabel? {
    get { return super.detailTextLabel }
    set { }
  }
  
  @available(*, unavailable)
  override var imageView: UIImageView? {
    get { return super.imageView }
    set { }
  }
  // swiftlint:enable unused_setter_value
}
