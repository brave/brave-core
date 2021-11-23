// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import SnapKit
import BraveUI
import BraveShared

class WelcomeViewSearchView: UIView {
    private struct DesignUX {
        static let padding = 16.0
        static let contentPadding = 30.0
        static let cornerRadius = 16.0
        static let buttonHeight = 48.0
        static let scrollViewWidth = BraveUX.baseDimensionValue
    }
    
    private let contentView = UIStackView().then {
        $0.axis = .vertical
        $0.spacing = DesignUX.padding
        $0.layoutMargins = UIEdgeInsets(equalInset: DesignUX.contentPadding)
        $0.isLayoutMarginsRelativeArrangement = true
        $0.setContentHuggingPriority(.required, for: .vertical)
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        addSubview(contentView)
        
        contentView.snp.makeConstraints {
            if traitCollection.horizontalSizeClass == .compact && traitCollection.verticalSizeClass == .regular {
                $0.edges.equalToSuperview()
            } else {
                $0.top.bottom.equalToSuperview()
                $0.centerX.equalToSuperview()
                $0.width.equalTo(DesignUX.scrollViewWidth + 2 * DesignUX.padding)            }
        }

    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    func addButton(icon: UIImage, title: String, action: @escaping () -> Void) {
        let button = SearchEngineButton(icon: icon, title: title).then {
            $0.addAction(UIAction(identifier: .init(rawValue: "primary.action"), handler: { _ in
                action()
            }), for: .primaryActionTriggered)
        }
        
        contentView.addArrangedSubview(button)
    }
}

private class SearchEngineButton: RoundInterfaceButton {
    struct DesignUX {
        static let cornerRadius = 40.0
        static let contentPaddingX = 15.0
        static let contentPaddingY = 10.0
        static let iconSize = 24.0
    }
    
    private let roundedLayer = CALayer()
    
    private let contentView = UIStackView().then {
        $0.spacing = 10.0
        $0.layoutMargins = UIEdgeInsets(top: DesignUX.contentPaddingY,
                                        left: DesignUX.contentPaddingX,
                                        bottom: DesignUX.contentPaddingY,
                                        right: DesignUX.contentPaddingX)
        $0.isUserInteractionEnabled = false
        $0.isLayoutMarginsRelativeArrangement = true
    }
    
    private let iconView = UIImageView().then {
        $0.contentMode = .scaleAspectFit
        $0.isUserInteractionEnabled = false
    }
    
    private let titleView = UILabel().then {
        $0.textAlignment = .left
        $0.textColor = .bravePrimary
        $0.font = .preferredFont(forTextStyle: .body)
        $0.isUserInteractionEnabled = false
    }
    
    private let accessoryView = UIImageView().then {
        $0.contentMode = .center
        $0.image = #imageLiteral(resourceName: "welcome-view-search-engine-arrow")
        $0.isUserInteractionEnabled = false
    }
    
    init(icon: UIImage, title: String) {
        super.init(frame: .zero)
        
        iconView.image = icon
        titleView.text = title
        backgroundColor = .clear
        
        contentMode = .left
        addSubview(contentView)
        [iconView, titleView, accessoryView].forEach {
            self.contentView.addArrangedSubview($0)
        }
        
        contentView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
        
        iconView.snp.makeConstraints {
            $0.width.equalTo(DesignUX.iconSize)
            $0.height.equalTo(DesignUX.iconSize)
        }
        
        accessoryView.snp.makeConstraints {
            $0.width.equalTo(DesignUX.iconSize)
            $0.height.equalTo(DesignUX.iconSize)
        }
        
        titleView.do {
            $0.setContentHuggingPriority(.defaultLow, for: .horizontal)
            $0.setContentCompressionResistancePriority(.required, for: .horizontal)
        }
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func layoutSubviews() {
        super.layoutSubviews()
        
        roundedLayer.do {
            $0.backgroundColor = UIColor.secondaryBraveBackground.cgColor
            $0.frame = contentView.bounds
            $0.mask = CAShapeLayer().then {
                $0.frame = contentView.bounds
                $0.path = UIBezierPath(roundedRect: contentView.bounds,
                                       byRoundingCorners: .allCorners,
                                       cornerRadii: CGSize(width: DesignUX.cornerRadius,
                                                           height: DesignUX.cornerRadius)).cgPath
            }
        }

        contentView.layer.insertSublayer(roundedLayer, at: 0)
        
        backgroundColor = .clear
        layer.shadowColor = UIColor.black.cgColor
        layer.shadowOpacity = 0.36
        layer.shadowOffset = CGSize(width: 0, height: 1)
        layer.shadowRadius = DesignUX.cornerRadius
        layer.shadowPath = UIBezierPath(roundedRect: bounds,
                                        cornerRadius: DesignUX.cornerRadius).cgPath
    }
}
