// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveShared
import Data

/// Displays a large favicon given some favorite
class LargeFaviconView: UIView {
    var domain: Domain? {
        didSet {
            if let domain = domain, let url = domain.url?.asURL, !url.absoluteString.isEmpty {
                // Use the base domain's first character, but if that isn't valid
                // use the favorites title as the monogram instead
                monogramFallbackLabel.text = FaviconFetcher.monogramLetter(
                    for: url,
                    fallbackCharacter: monogramFallbackCharacter
                )
                // Setup the favicon fetcher to pull a large icon for the given
                // domain
                fetcher = FaviconFetcher(siteURL: url, kind: .largeIcon, domain: domain)
                fetcher?.load { [weak self] _, attributes in
                    guard let self = self, self.domain?.url?.asURL == url else { return }
                    self.monogramFallbackLabel.isHidden = attributes.image != nil
                    self.imageView.image = attributes.image
                    self.imageView.contentMode = attributes.contentMode
                    self.backgroundColor = attributes.backgroundColor
                    self.layoutMargins = .init(equalInset: attributes.includePadding ? 4 : 0)
                    self.backgroundView.isHidden = !attributes.includePadding
                }
            } else {
                fetcher = nil
                monogramFallbackLabel.text = nil
                monogramFallbackLabel.isHidden = true
                imageView.image = nil
            }
        }
    }
    
    /// The character that should be displayed if none can be assumed from
    /// `domain`.
    var monogramFallbackCharacter: Character? {
        didSet {
            if monogramFallbackLabel.text == nil {
                monogramFallbackLabel.text = monogramFallbackCharacter?.uppercased()
            }
        }
    }
    
    private var fetcher: FaviconFetcher?
    
    private let imageView = UIImageView().then {
        $0.contentMode = .scaleAspectFit
    }
    
    private let monogramFallbackLabel = UILabel().then {
        $0.appearanceTextColor = .white
        $0.isHidden = true
    }
    
    override func layoutSubviews() {
        super.layoutSubviews()
        
        if bounds.height > 0 {
            monogramFallbackLabel.font = .systemFont(ofSize: bounds.height / 2)
        }
    }
    
    private let backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .regular)).then {
        $0.isHidden = true
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        layer.cornerRadius = 8
        if #available(iOS 13.0, *) {
            layer.cornerCurve = .continuous
        }
        
        clipsToBounds = true
        layer.borderColor = BraveUX.faviconBorderColor.cgColor
        layer.borderWidth = BraveUX.faviconBorderWidth
        
        layoutMargins = .zero
        
        addSubview(backgroundView)
        addSubview(monogramFallbackLabel)
        addSubview(imageView)
        
        backgroundView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
        
        imageView.snp.makeConstraints {
            $0.center.equalTo(self)
            $0.leading.top.greaterThanOrEqualTo(layoutMarginsGuide)
            $0.trailing.bottom.lessThanOrEqualTo(layoutMarginsGuide)
        }
        monogramFallbackLabel.snp.makeConstraints {
            $0.center.equalTo(self)
        }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
}
