// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI

class PlaylistURLBarButton: UIButton {
    enum State {
        case addToPlaylist
        case addedToPlaylist
        case none
    }
    
    var buttonState: State = .none {
        didSet {
            switch buttonState {
            case .addToPlaylist:
                setImage(#imageLiteral(resourceName: "playlist_toolbar_add_button"), for: .normal)
                gradientView.isHidden = false
                backgroundView.isHidden = true
                isHidden = false
            case .addedToPlaylist:
                setImage(#imageLiteral(resourceName: "playlist_toolbar_added_button"), for: .normal)
                gradientView.isHidden = true
                backgroundView.isHidden = false
                isHidden = false
            case .none:
                setImage(nil, for: .normal)
                gradientView.isHidden = true
                backgroundView.isHidden = true
                isHidden = true
            }
        }
    }
    
    private let gradientView = BraveGradientView(gradient: .lightGradient02)
    private let backgroundView = UIView().then {
        $0.backgroundColor = .braveSeparator
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        imageView?.contentMode = .center
        insertSubview(gradientView, at: 0)
        insertSubview(backgroundView, belowSubview: gradientView)
        
        [backgroundView, gradientView].forEach {
            $0.do {
                $0.layer.cornerRadius = 13.0
                $0.layer.masksToBounds = true
                $0.isUserInteractionEnabled = false
                
                $0.snp.makeConstraints {
                    $0.center.equalToSuperview()
                    $0.leading.equalToSuperview()
                    $0.trailing.equalToSuperview().inset(5.0)
                    $0.width.equalTo(40.0)
                    $0.height.equalTo(26.0)
                }
            }
        }
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
}
