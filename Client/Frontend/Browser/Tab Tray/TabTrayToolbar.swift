// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import BraveUI

class TrayToolbar: UIView {
    fileprivate let toolbarButtonSize = CGSize(width: 44, height: 44)
    
    let addTabButton = UIButton(type: .system).then {
        $0.setImage(#imageLiteral(resourceName: "add_tab").template, for: .normal)
        $0.accessibilityLabel = Strings.tabTrayAddTabAccessibilityLabel
        $0.accessibilityIdentifier = "TabTrayController.addTabButton"
        $0.tintColor = .braveLabel
    }
    
    let doneButton = UIButton(type: .system).then {
        $0.setTitle(Strings.done, for: .normal)
        $0.titleLabel?.font = TabTrayControllerUX.toolbarFont
        $0.accessibilityLabel = Strings.done
        $0.accessibilityIdentifier = "TabTrayController.doneButton"
        $0.tintColor = .braveLabel
    }
    
    let privateModeButton = PrivateModeButton().then {
        $0.titleLabel?.font = TabTrayControllerUX.toolbarFont
        $0.setTitle(Strings.private, for: .normal)
        $0.tintColor = .braveLabel
    }
    
    fileprivate let sideOffset: CGFloat = 22
    
    fileprivate override init(frame: CGRect) {
        super.init(frame: frame)
        
        backgroundColor = .secondaryBraveBackground
        
        addSubview(addTabButton)
        addSubview(doneButton)
        
        var buttonToCenter: UIButton?
        buttonToCenter = addTabButton
        
        privateModeButton.accessibilityIdentifier = "TabTrayController.maskButton"
        
        buttonToCenter?.snp.makeConstraints { make in
            make.centerX.equalTo(self)
            make.top.equalTo(self)
            make.size.equalTo(toolbarButtonSize)
        }
        
        doneButton.snp.makeConstraints { make in
            make.centerY.equalTo(safeArea.centerY)
            make.trailing.equalTo(self).offset(-sideOffset)
        }
        
        addSubview(privateModeButton)
        privateModeButton.snp.makeConstraints { make in
            make.centerY.equalTo(safeArea.centerY)
            make.leading.equalTo(self).offset(sideOffset)
        }
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
}
