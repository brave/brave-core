// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import Shared
import BraveShared

class DefaultBrowserCalloutProvider: NSObject, NTPObservableSectionProvider {
    var sectionDidChange: (() -> Void)?
    
    private typealias DefaultBrowserCalloutCell = NewTabCenteredCollectionViewCell<DefaultBrowserCalloutView>
    
    static var shouldShowCallout: Bool {
        !Preferences.General.defaultBrowserCalloutDismissed.value
            && AppConstants.iOSVersionGreaterThanOrEqual(to: 14)
            && AppConstants.buildChannel == .release
    }
    
    func registerCells(to collectionView: UICollectionView) {
        collectionView.register(DefaultBrowserCalloutCell.self)
    }
    
    func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
        Self.shouldShowCallout ? 1 : 0
    }
    
    func collectionView(_ collectionView: UICollectionView,
                        cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
        let cell = collectionView.dequeueReusableCell(for: indexPath) as DefaultBrowserCalloutCell
        cell.view.addTarget(self, action: #selector(openSettings), for: .touchUpInside)
        cell.view.closeHaandler = { [weak self] in
            Preferences.General.defaultBrowserCalloutDismissed.value = true
            self?.sectionDidChange?()
            
        }
        return cell
    }
    
    func collectionView(_ collectionView: UICollectionView,
                        layout collectionViewLayout: UICollectionViewLayout,
                        sizeForItemAt indexPath: IndexPath) -> CGSize {
        
        return fittingSizeForCollectionView(collectionView, section: indexPath.section)
    }
    
    func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, insetForSectionAt section: Int) -> UIEdgeInsets {
        if !Self.shouldShowCallout {
            return .zero
        }
        
        return UIEdgeInsets(top: 12, left: 16, bottom: -16, right: 16)
    }
    
    @objc func openSettings() {
        guard let settingsUrl = URL(string: UIApplication.openSettingsURLString) else {
            return
        }
        
        Preferences.General.defaultBrowserCalloutDismissed.value = true
        UIApplication.shared.open(settingsUrl)
    }
}

private class DefaultBrowserCalloutView: SpringButton, Themeable {
    
    var closeHaandler: (() -> Void)?
    
    private let closeButton = UIButton().then {
        $0.setImage(#imageLiteral(resourceName: "close_tab_bar").template, for: .normal)
        $0.tintColor = .lightGray
        $0.contentEdgeInsets = UIEdgeInsets(equalInset: 4)
        $0.accessibilityLabel = Strings.defaultBrowserCalloutCloseAccesabilityLabel
    }
    
    private let label = UILabel().then {
        $0.text = Strings.setDefaultBrowserCalloutTitle
        $0.appearanceTextColor = .black
        $0.font = UIFont.systemFont(ofSize: 14.0, weight: .medium)
        $0.numberOfLines = 0
        $0.preferredMaxLayoutWidth = 280
        $0.textAlignment = .center
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        clipsToBounds = true
        layer.cornerRadius = 8
        backgroundColor = #colorLiteral(red: 0.8588235294, green: 0.9568627451, blue: 0.862745098, alpha: 1)
        
        addSubview(label)
        addSubview(closeButton)
        
        closeButton.addTarget(self, action: #selector(closeTab), for: .touchUpInside)
        
        label.snp.makeConstraints {
            $0.top.bottom.equalToSuperview().inset(10)
            $0.leading.equalToSuperview().offset(24)
            $0.trailing.equalTo(closeButton).inset(20)
        }
        
        closeButton.snp.makeConstraints {
            $0.top.equalToSuperview().inset(2)
            $0.trailing.equalToSuperview().inset(8)
        }
    }
    
    @objc func closeTab() {
        closeHaandler?()
    }

}
