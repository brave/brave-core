/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SnapKit
import Shared

private struct TabsButtonUX {
    static let cornerRadius: CGFloat = 3
    static let titleFont: UIFont = UIConstants.defaultChromeSmallFontBold
    static let borderStrokeWidth: CGFloat = 1.5
}

class TabsButton: UIButton {
    
    private let countLabel = UILabel().then {
        $0.font = TabsButtonUX.titleFont
        $0.textAlignment = .center
        $0.isUserInteractionEnabled = false
        $0.textColor = .braveLabel
    }
    
    private let borderView = UIView().then {
        $0.layer.borderWidth = TabsButtonUX.borderStrokeWidth
        $0.layer.cornerRadius = TabsButtonUX.cornerRadius
        $0.layer.cornerCurve = .continuous
        $0.isUserInteractionEnabled = false
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        accessibilityTraits.insert(.button)
        isAccessibilityElement = true
        accessibilityLabel = Strings.showTabs
        
        addSubview(borderView)
        addSubview(countLabel)
        
        countLabel.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
        borderView.snp.makeConstraints {
            $0.center.equalToSuperview()
            $0.size.equalTo(19)
        }
        
        borderView.layer.borderColor = UIColor.braveLabel.resolvedColor(with: traitCollection).cgColor
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    override var isHighlighted: Bool {
        didSet {
            let color: UIColor = isHighlighted ? .braveOrange : .braveLabel
            countLabel.textColor = color
            borderView.layer.borderColor = color.resolvedColor(with: traitCollection).cgColor
        }
    }
    
    override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
        super.traitCollectionDidChange(previousTraitCollection)
        // CGColor's do not get automatic updates
        borderView.layer.borderColor = isHighlighted ? UIColor.braveOrange.cgColor :
            UIColor.braveLabel.resolvedColor(with: traitCollection).cgColor
    }
    
    private var currentCount: Int?
    
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
