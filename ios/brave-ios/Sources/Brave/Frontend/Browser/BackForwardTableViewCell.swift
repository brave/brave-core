// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Favicon
import Shared
import Storage
import UIKit

class BackForwardTableViewCell: UITableViewCell {

  private struct BackForwardViewCellUX {
    static let faviconWidth = 29
    static let faviconPadding: CGFloat = 20
    static let labelPadding = 20
    static let borderSmall = 2
    static let borderBold = 5
    static let iconSize = 23
    static let fontSize: CGFloat = 12.0
    static let textColor: UIColor = .braveLabel
  }

  lazy var faviconView: UIImageView = {
    let faviconView = UIImageView(image: UIImage(sharedNamed: "brave.logo"))
    faviconView.backgroundColor = .braveBackground
    faviconView.layer.cornerRadius = 6
    faviconView.layer.cornerCurve = .continuous
    faviconView.layer.borderWidth = 0.5
    faviconView.layer.borderColor = UIColor(white: 0, alpha: 0.1).cgColor
    faviconView.layer.masksToBounds = true
    faviconView.contentMode = .scaleAspectFit
    return faviconView
  }()

  let line = UIView().then {
    $0.backgroundColor = .braveSeparator
  }

  lazy var label: UILabel = {
    let label = UILabel()
    label.text = " "
    label.font = label.font.withSize(BackForwardViewCellUX.fontSize)
    label.textColor = BackForwardViewCellUX.textColor
    return label
  }()

  var isPrivateBrowsing: Bool = false

  var connectingForwards = true {
    didSet {
      setNeedsUpdateConstraints()
    }
  }
  var connectingBackwards = true {
    didSet {
      setNeedsUpdateConstraints()
    }
  }

  var isCurrentTab = false {
    didSet {
      if isCurrentTab {
        label.font = UIFont(name: "HelveticaNeue-Bold", size: BackForwardViewCellUX.fontSize)
        contentView.backgroundColor = .secondaryBraveBackground
      }
    }
  }

  var site: Site? {
    didSet {
      if let s = site {
        if InternalURL.isValid(url: s.tileURL) {
          faviconView.backgroundColor = .white
          faviconView.image = UIImage(sharedNamed: "brave.logo")
        } else {
          faviconView.loadFavicon(for: s.tileURL, isPrivateBrowsing: isPrivateBrowsing)
        }
        var title = s.title
        if title.isEmpty {
          title = s.url
        }
        label.text = title
        setNeedsLayout()
      }
    }
  }

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)
    backgroundColor = .clear
    selectionStyle = .none

    contentView.addSubview(line)
    contentView.addSubview(faviconView)
    contentView.addSubview(label)

    faviconView.snp.makeConstraints { make in
      make.height.equalTo(BackForwardViewCellUX.faviconWidth)
      make.width.equalTo(BackForwardViewCellUX.faviconWidth)
      make.centerY.equalTo(self)
      make.leading.equalTo(self.safeArea.leading).offset(BackForwardViewCellUX.faviconPadding)
    }

    label.snp.makeConstraints { make in
      make.centerY.equalTo(self)
      make.leading.equalTo(faviconView.snp.trailing).offset(BackForwardViewCellUX.labelPadding)
      make.trailing.equalTo(self.safeArea.trailing).offset(-BackForwardViewCellUX.labelPadding)
    }
  }

  override func updateConstraints() {
    super.updateConstraints()

    line.snp.remakeConstraints {
      if connectingForwards {
        $0.top.equalToSuperview()
      } else {
        $0.top.equalTo(faviconView.snp.centerY)
      }
      if connectingBackwards {
        $0.bottom.equalToSuperview()
      } else {
        $0.bottom.equalTo(faviconView.snp.centerY)
      }
      $0.centerX.equalTo(faviconView)
      $0.width.equalTo(1.0 / UIScreen.main.scale)
    }
  }

  required init(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func setHighlighted(_ highlighted: Bool, animated: Bool) {
    if highlighted {
      self.backgroundColor = UIColor(white: 0, alpha: 0.1)
    } else {
      self.backgroundColor = .clear
    }
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    connectingForwards = true
    connectingBackwards = true
    isCurrentTab = false
    label.font = UIFont(name: "HelveticaNeue", size: BackForwardViewCellUX.fontSize)
  }
}
