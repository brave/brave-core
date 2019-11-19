/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SnapKit

private struct ErrorToastDefaultUX {
    static let cornerRadius: CGFloat = 40
    static let margins = UIEdgeInsets(top: 10, left: 12, bottom: 10, right: 12)
    static let textColor = UIColor(rgb: 0xBD1531)
}

class ErrorToast: UIView {
    lazy var textLabel: UILabel = {
        let label = UILabel()
        label.appearanceTextColor = ErrorToastDefaultUX.textColor
        label.appearanceBackgroundColor = .clear
        label.textAlignment = .center
        label.numberOfLines = 0
        return label
    }()

    var cornerRadius: CGFloat = ErrorToastDefaultUX.cornerRadius {
        didSet {
            setNeedsDisplay()
        }
    }

    override init(frame: CGRect) {
        super.init(frame: frame)
        isOpaque = false
        addSubview(textLabel)
        textLabel.snp.makeConstraints { make in
            make.edges.equalTo(self).inset(ErrorToastDefaultUX.margins)
        }
    }

    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
}
