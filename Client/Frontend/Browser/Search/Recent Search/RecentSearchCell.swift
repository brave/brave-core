// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import SnapKit
import BraveUI

class RecentSearchCell: UICollectionViewCell, CollectionViewReusable {
    static let identifier = "RecentSearchCell"
    
    private let stackView = UIStackView().then {
        $0.spacing = 20.0
        $0.isLayoutMarginsRelativeArrangement = true
        $0.layoutMargins = UIEdgeInsets(top: 0.0, left: 12.0, bottom: 0.0, right: 12.0)
    }
    
    private let titleLabel = UILabel().then {
        $0.font = .systemFont(ofSize: 15.0)
    }
    
    private let openButton = UIImageView().then {
        $0.image = #imageLiteral(resourceName: "recent-search-arrow")
        $0.contentMode = .scaleAspectFit
        $0.setContentHuggingPriority(.required, for: .horizontal)
        $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        contentView.addSubview(stackView)
        stackView.addArrangedSubview(titleLabel)
        stackView.addArrangedSubview(openButton)
        
        stackView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    func setTitle(_ title: String?) {
        titleLabel.text = title
    }
    
    func setAttributedTitle(_ title: NSAttributedString?) {
        titleLabel.attributedText = title
    }
}
