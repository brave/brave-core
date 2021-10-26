// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared

extension LoginListViewController {
    class LoginEmptyView: UIView {
        
        private let titleLabel = UILabel().then {
            $0.text = Strings.Login.loginListEmptyScreenTitle
            $0.numberOfLines = 0
            $0.font = .preferredFont(for: .title3, weight: .semibold)
            $0.adjustsFontForContentSizeCategory = true
            $0.textColor = .bravePrimary
        }
                
        override init(frame: CGRect) {
            super.init(frame: frame)
            
            backgroundColor = .secondaryBraveBackground
      
            addSubview(titleLabel)
            titleLabel.snp.remakeConstraints { make in
                make.centerX.equalTo(self)
                make.centerY.equalTo(self).offset(-(2 * UX.headerHeight))
            }
        }
        
        @available(*, unavailable)
        required init?(coder aDecoder: NSCoder) {
            fatalError("init(coder:) has not been implemented")
        }
    }
}
