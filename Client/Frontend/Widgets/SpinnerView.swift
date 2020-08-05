// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import Shared

class SpinnerView: UIView {
    
    struct DefaultUX {
        static let backgroundColor = UIColor.black.withAlphaComponent(0.25)
        static let spinnerBackgroundColor = UIColor.black.withAlphaComponent(0.60)
        static let cornerRadius = 10.0
        static let animationSpeed = 0.25
        static let insets = 20.0
    }
    
    private let container = UIStackView().then {
        $0.axis = .vertical
        $0.alignment = .center
        $0.spacing = 6.0
        $0.isLayoutMarginsRelativeArrangement = true
        $0.layoutMargins = UIEdgeInsets(equalInset: 15.0)
    }
    
    private let backgroundView = UIView().then {
        $0.layer.masksToBounds = true
        $0.layer.cornerRadius = CGFloat(DefaultUX.cornerRadius)
        $0.backgroundColor = DefaultUX.spinnerBackgroundColor
    }
    
    private let activityView = UIActivityIndicatorView(style: .whiteLarge).then {
        $0.color = .white
        $0.startAnimating()
    }
    
    private let textLabel = UILabel().then {
        $0.font = .systemFont(ofSize: 16.0, weight: .medium)
        $0.textColor = .white
        $0.text = Strings.clearingData
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        self.backgroundColor = DefaultUX.backgroundColor
        
        addSubview(backgroundView)
        backgroundView.addSubview(container)
        
        backgroundView.snp.makeConstraints {
            $0.center.equalToSuperview()
            $0.leading.greaterThanOrEqualTo(self.safeArea.leading).offset(DefaultUX.insets)
            $0.trailing.lessThanOrEqualTo(self.safeArea.trailing).offset(-DefaultUX.insets)
            $0.top.greaterThanOrEqualTo(self.safeArea.top).offset(DefaultUX.insets)
            $0.bottom.lessThanOrEqualTo(self.safeArea.bottom).offset(-DefaultUX.insets)
            $0.width.equalTo(backgroundView.snp.height)
            $0.height.equalTo(backgroundView.snp.width)
        }
        
        container.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
        
        container.addArrangedSubview(activityView)
        container.addArrangedSubview(textLabel)
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    public func present(on view: UIView) {
        if self.superview != nil {
            return
        }
        
        self.alpha = 0.0
        
        view.window?.addSubview(self)
        self.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
        
        UIView.animate(withDuration: DefaultUX.animationSpeed) {
            self.alpha = 1.0
        }
    }
    
    public func dismiss() {
        if self.superview == nil {
            return
        }
        
        self.alpha = 1.0
        UIView.animate(withDuration: DefaultUX.animationSpeed, animations: {
            self.alpha = 0.0
        }, completion: { _ in
            self.removeFromSuperview()
        })
    }
}
