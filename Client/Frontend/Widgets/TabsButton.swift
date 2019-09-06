/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SnapKit
import Shared

private struct TabsButtonUX {
    static let CornerRadius: CGFloat = 2
    static let TitleFont: UIFont = UIConstants.DefaultChromeSmallFontBold
    static let BorderStrokeWidth: CGFloat = 1.5
}

class TabsButton: UIButton {

    var textColor = UIColor.Photon.White100 {
        didSet {
            updateButtonVisuals()
        }
    }
    
    // Explicit, should crash if not setup properly
    var highlightTextColor: UIColor!
    
    private var currentCount: Int?
    private var top: Bool

    override var isHighlighted: Bool {
        didSet {
            updateButtonVisuals()
        }
    }
    
    private func updateButtonVisuals() {
        let foregroundColor: UIColor = isHighlighted ? highlightTextColor : textColor
        countLabel.textColor = foregroundColor
        borderView.color = foregroundColor
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
        background.backgroundColor = .clear
        return background
    }()

    fileprivate lazy var borderView: InnerStrokedView = {
        let border = InnerStrokedView()
        border.strokeWidth = TabsButtonUX.BorderStrokeWidth
        border.cornerRadius = TabsButtonUX.CornerRadius
        border.isUserInteractionEnabled = false
        return border
    }()
    
    required init(top: Bool) {
        self.top = top
        super.init(frame: .zero)
        [labelBackground, borderView, countLabel].forEach(insideButton.addSubview)
        addSubview(insideButton)
        isAccessibilityElement = true
        accessibilityTraits.insert(.button)
        self.accessibilityLabel = Strings.Show_Tabs
    }
    
    override init(frame: CGRect) {
        fatalError("init(coder:) has not been implemented")
    }

    override func updateConstraints() {
        super.updateConstraints()
        [labelBackground, borderView, countLabel].forEach {
            $0.snp.remakeConstraints { make in
                make.edges.equalTo(insideButton)
            }
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
        styleChildren(theme: theme)
        
        textColor = top ? theme.colors.tints.header : theme.colors.tints.footer
        highlightTextColor = theme.colors.accent
    }
}

