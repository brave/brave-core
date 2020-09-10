// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import Shared
import BraveShared

private class DuckDuckGoCalloutButton: SpringButton, Themeable {
    fileprivate struct UX {
        static let logoSize = CGSize(width: 38, height: 38)
        static let padding = UIEdgeInsets(top: 10, left: 10, bottom: 10, right: 20)
    }
    
    private let backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .dark)).then {
        $0.clipsToBounds = true
        $0.isUserInteractionEnabled = false
        $0.layer.cornerRadius = 4
    }
    
    private let logoImageView = UIImageView(image: #imageLiteral(resourceName: "duckduckgo"))
    
    private let label = UILabel().then {
        $0.text = Strings.DDGPromotion
        $0.numberOfLines = 0
        $0.preferredMaxLayoutWidth = 180
        $0.appearanceTextColor = .white
        $0.font = UIFont.systemFont(ofSize: 13.0, weight: .bold)
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        let stackView = UIStackView(arrangedSubviews: [logoImageView, label]).then {
            $0.spacing = 12
            $0.alignment = .center
        }
        
        addSubview(backgroundView)
        backgroundView.contentView.addSubview(stackView)
        
        backgroundView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
        logoImageView.snp.makeConstraints {
            $0.size.equalTo(UX.logoSize)
        }
        stackView.snp.makeConstraints {
            $0.edges.equalToSuperview().inset(UX.padding)
        }
    }
    
    override func layoutSubviews() {
        super.layoutSubviews()
        backgroundView.layer.cornerRadius = bounds.height / 2.0 // Pill shape
    }
}

class DuckDuckGoCalloutSectionProvider: NSObject, NTPObservableSectionProvider {
    var sectionDidChange: (() -> Void)?
    private let profile: Profile
    private let action: () -> Void
    
    private typealias DuckDuckGoCalloutCell = NewTabCenteredCollectionViewCell<DuckDuckGoCalloutButton>
    
    init(profile: Profile, action: @escaping () -> Void) {
        self.profile = profile
        self.action = action
        super.init()
        
        NotificationCenter.default.addObserver(self, selector: #selector(privateModeChanged), name: .privacyModeChanged, object: nil)
    }
    
    deinit {
        NotificationCenter.default.removeObserver(self)
    }
    
    private var isShowingCallout: Bool {
        let isSearchEngineSet = profile.searchEngines.defaultEngine(forType: .privateMode).shortName == OpenSearchEngine.EngineNames.duckDuckGo
        let isPrivateBrowsing = PrivateBrowsingManager.shared.isPrivateBrowsing
        let shouldShowPromo = SearchEngines.shouldShowDuckDuckGoPromo
        return isPrivateBrowsing && !isSearchEngineSet && shouldShowPromo
    }
    
    @objc private func privateModeChanged() {
        sectionDidChange?()
    }
    
    @objc private func tappedButton() {
        action()
    }
    
    func registerCells(to collectionView: UICollectionView) {
        collectionView.register(DuckDuckGoCalloutCell.self)
    }
    
    func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
        let cell = collectionView.dequeueReusableCell(for: indexPath) as DuckDuckGoCalloutCell
        cell.view.addTarget(self, action: #selector(tappedButton), for: .touchUpInside)
        return cell
    }
    
    func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
        var size = fittingSizeForCollectionView(collectionView, section: indexPath.section)
        size.height = DuckDuckGoCalloutButton.UX.logoSize.height +
            DuckDuckGoCalloutButton.UX.padding.top +
            DuckDuckGoCalloutButton.UX.padding.bottom
        return size
    }
    
    func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
        return isShowingCallout ? 1 : 0
    }
    
    func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, insetForSectionAt section: Int) -> UIEdgeInsets {
        if !isShowingCallout {
            return .zero
        }
        return UIEdgeInsets(top: 60.0, left: 16, bottom: 16, right: 16)
    }
}
