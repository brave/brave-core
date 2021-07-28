// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI
import SnapKit
import BraveShared

class OpenSearchEngineButton: BraveButton {

    // MARK: Action
    
    enum Action {
        case loading
        case enabled
        case disabled
    }

    // MARK: Properties
    
    var action: Action {
        didSet {
            switch action {
            case .disabled:
                isLoading = false
                setImage(#imageLiteral(resourceName: "AddSearch").template, for: .normal)
                tintColor = .braveDisabled
                isUserInteractionEnabled = false
            case .enabled:
                isLoading = false
                setImage(#imageLiteral(resourceName: "AddSearch").template, for: .normal)
                tintColor = .braveOrange
                isUserInteractionEnabled = true
            case .loading:
                isLoading = true
                setImage(nil, for: .normal)
            }
        }
    }
    
    private var hidesWhenDisabled: Bool = false
    
    // MARK: Lifecycle
    
    override init(frame: CGRect) {
        self.action = .disabled
        super.init(frame: frame)
        setTitleColor(.braveOrange, for: .normal)
    }

    convenience init(title: String? = nil, hidesWhenDisabled: Bool) {
        self.init(frame: CGRect(x: 0, y: 0, width: 44.0, height: 44.0))
        
        self.hidesWhenDisabled = hidesWhenDisabled
        
        setTheme(with: title)
    }

    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    // MARK: Internal
    
    private func setTheme(with title: String?) {
        setImage(#imageLiteral(resourceName: "AddSearch").template, for: [])
        titleLabel?.font = UIFont.preferredFont(forTextStyle: .footnote)
        
        if let title = title {
            setImage(nil, for: .normal)
            setTitle(title, for: .normal)
        }
        
        loaderView = LoaderView(size: .small).then {
            $0.tintColor = .braveLabel
        }
    }
}

