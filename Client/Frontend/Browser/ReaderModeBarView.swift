/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import SnapKit
import Shared
import XCGLogger

private let log = Logger.browserLogger

enum ReaderModeBarButtonType {
    case settings

    fileprivate var localizedDescription: String {
        switch self {
        case .settings: return Strings.readerModeDisplaySettingsButtonTitle
        }
    }

    fileprivate var imageName: String {
        switch self {
        case .settings: return "SettingsSerif"
        }
    }

    fileprivate var image: UIImage? {
        let image = UIImage(imageLiteralResourceName: imageName)
        image.accessibilityLabel = localizedDescription
        return image
    }
}

protocol ReaderModeBarViewDelegate {
    func readerModeBar(_ readerModeBar: ReaderModeBarView, didSelectButton buttonType: ReaderModeBarButtonType)
}

class ReaderModeBarView: UIView {
    var delegate: ReaderModeBarViewDelegate?

    var settingsButton: UIButton!

    override init(frame: CGRect) {
        super.init(frame: frame)
        
        backgroundColor = .secondaryBraveBackground

        settingsButton = createButton(.settings, action: #selector(tappedSettingsButton))
        settingsButton.accessibilityIdentifier = "ReaderModeBarView.settingsButton"
        settingsButton.snp.makeConstraints { (make) -> Void in
            make.height.centerX.centerY.equalTo(self)
            make.width.equalTo(80)
        }
        settingsButton.tintColor = .braveLabel
        
        let borderView = UIView.separatorLine
        addSubview(borderView)
        borderView.snp.makeConstraints {
            $0.top.equalTo(snp.bottom)
            $0.leading.trailing.equalToSuperview()
        }
    }

    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    fileprivate func createButton(_ type: ReaderModeBarButtonType, action: Selector) -> UIButton {
        let button = UIButton()
        addSubview(button)
        button.setImage(type.image, for: [])
        button.addTarget(self, action: action, for: .touchUpInside)
        return button
    }

    @objc func tappedSettingsButton(_ sender: UIButton!) {
        delegate?.readerModeBar(self, didSelectButton: .settings)
    }
}
