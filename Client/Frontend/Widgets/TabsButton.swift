/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SnapKit
import Shared

private struct TabsButtonUX {
    static let TitleColor: UIColor = UIColor.Photon.Grey80
    static let TitleBackgroundColor: UIColor = UIColor.Photon.White100
    static let CornerRadius: CGFloat = 2
    static let TitleFont: UIFont = UIConstants.DefaultChromeSmallFontBold
    static let BorderStrokeWidth: CGFloat = 1.5
}

class TabsButton: UIButton {

    var textColor = UIColor.Photon.White100 {
        didSet {
            countLabel.textColor = textColor
            borderView.color = textColor
        }
    }
    var titleBackgroundColor  = UIColor.Photon.White100 {
        didSet {
            labelBackground.backgroundColor = titleBackgroundColor
        }
    }
    var highlightTextColor: UIColor?
    var highlightBackgroundColor: UIColor?
    
    private var currentCount: Int?

    override var isHighlighted: Bool {
        didSet {
            if isHighlighted {
                countLabel.textColor = textColor
                borderView.color = titleBackgroundColor
                labelBackground.backgroundColor = titleBackgroundColor
            } else {
                countLabel.textColor = textColor
                borderView.color = textColor
                labelBackground.backgroundColor = titleBackgroundColor
            }
        }
    }

    lazy var countLabel: UILabel = {
        let label = UILabel()
        label.font = TabsButtonUX.TitleFont
        label.layer.cornerRadius = TabsButtonUX.CornerRadius
        label.textAlignment = .center
        label.isUserInteractionEnabled = false
        return label
    }()

    lazy var insideButton: UIView = {
        let view = UIView()
        view.clipsToBounds = false
        view.isUserInteractionEnabled = false
        return view
    }()

    fileprivate lazy var labelBackground: UIView = {
        let background = UIView()
        background.layer.cornerRadius = TabsButtonUX.CornerRadius
        background.isUserInteractionEnabled = false
        return background
    }()

    fileprivate lazy var borderView: InnerStrokedView = {
        let border = InnerStrokedView()
        border.strokeWidth = TabsButtonUX.BorderStrokeWidth
        border.cornerRadius = TabsButtonUX.CornerRadius
        border.isUserInteractionEnabled = false
        return border
    }()

    override init(frame: CGRect) {
        super.init(frame: frame)
        insideButton.addSubview(labelBackground)
        insideButton.addSubview(borderView)
        insideButton.addSubview(countLabel)
        addSubview(insideButton)
        isAccessibilityElement = true
        accessibilityTraits.insert(.button) 
        self.accessibilityLabel = Strings.Show_Tabs
    }

    override func updateConstraints() {
        super.updateConstraints()
        labelBackground.snp.remakeConstraints { (make) -> Void in
            make.edges.equalTo(insideButton)
        }
        borderView.snp.remakeConstraints { (make) -> Void in
            make.edges.equalTo(insideButton)
        }
        countLabel.snp.remakeConstraints { (make) -> Void in
            make.edges.equalTo(insideButton)
        }
        insideButton.snp.remakeConstraints { (make) -> Void in
            make.size.equalTo(19)
            make.center.equalTo(self)
        }
    }

    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    func updateTabCount(_ count: Int) {
        let count = max(count, 1)
        // Sometimes tabs count state is held in the cloned tabs button.
        let infinity = "\u{221E}"
        let countToBe = (count < 100) ? "\(count)" : infinity
        currentCount = count
        self.countLabel.text = countToBe
        self.accessibilityValue = countToBe
    }
}

extension TabsButton: Themeable {
    func applyTheme(_ theme: Theme) {
        titleBackgroundColor = UIColor.Browser.Background.colorFor(theme)
        textColor = UIColor.Browser.Tint.colorFor(theme)
        countLabel.textColor = UIColor.Browser.Tint.colorFor(theme)
        borderView.color = UIColor.Browser.Tint.colorFor(theme)
        labelBackground.backgroundColor = UIColor.Browser.Background.colorFor(theme)
    }
}

