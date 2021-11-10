// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import BraveUI

extension TabTrayController {
    
    class View: UIView {
        private struct UX {
            static let regularCellHeight = 192.0
            static let largeCellHeight = 256.0
        }
        
        private func generateLayout(numberOfColumns: Int = 2,
                                    cellHeight: CGFloat = UX.regularCellHeight) -> UICollectionViewLayout {
            let itemSize = NSCollectionLayoutSize(
                widthDimension: .fractionalWidth(1.0),
                heightDimension: .fractionalHeight(1.0))
            let item = NSCollectionLayoutItem(layoutSize: itemSize)
            item.contentInsets = .init(top: 6, leading: 6, bottom: 6, trailing: 6)
            
            let groupSize = NSCollectionLayoutSize(
                widthDimension: .fractionalWidth(1.0),
                heightDimension: .absolute(cellHeight))
            let group = NSCollectionLayoutGroup.horizontal(
                layoutSize: groupSize,
                subitem: item,
                count: numberOfColumns)
            
            let section = NSCollectionLayoutSection(group: group)
            let layout = UICollectionViewCompositionalLayout(section: section)
            return layout
        }
        
        lazy var collectionView = UICollectionView(frame: .zero, collectionViewLayout: generateLayout()).then {
            $0.setContentHuggingPriority(.defaultLow, for: .vertical)
            $0.backgroundColor = .secondaryBraveBackground
        }
        
        let newTabButton = UIButton(type: .system).then {
            $0.setImage(#imageLiteral(resourceName: "add_tab").template, for: .normal)
            $0.accessibilityLabel = Strings.tabTrayAddTabAccessibilityLabel
            $0.accessibilityIdentifier = "TabTrayController.addTabButton"
            $0.tintColor = .braveLabel
            $0.contentEdgeInsets = .init(top: 0, left: 10, bottom: 0, right: 10)
            $0.setContentCompressionResistancePriority(.required, for: .horizontal)
        }
        
        let doneButton = UIButton(type: .system).then {
            $0.setTitle(Strings.done, for: .normal)
            $0.titleLabel?.font = .preferredFont(forTextStyle: .body)
            $0.titleLabel?.adjustsFontForContentSizeCategory = true
            $0.contentHorizontalAlignment = .right
            $0.titleLabel?.adjustsFontSizeToFitWidth = true
            $0.accessibilityLabel = Strings.done
            $0.accessibilityIdentifier = "TabTrayController.doneButton"
            $0.tintColor = .braveLabel
        }
        
        let privateModeButton = PrivateModeButton().then {
            $0.titleLabel?.font = .preferredFont(forTextStyle: .body)
            $0.titleLabel?.adjustsFontForContentSizeCategory = true
            $0.titleLabel?.adjustsFontSizeToFitWidth = true
            $0.contentHorizontalAlignment = .left
            $0.setTitle(Strings.private, for: .normal)
            $0.tintColor = .braveLabel
                    
            if Preferences.Privacy.privateBrowsingOnly.value {
                $0.alpha = 0
            }
        }
        
        private let privateModeInfo = TabTrayPrivateModeInfoView().then {
            $0.learnMoreButton.addTarget(self, action: #selector(privateModeLearnMoreAction),
                                         for: .touchUpInside)
            $0.setContentHuggingPriority(.defaultLow, for: .vertical)
            $0.isHidden = true
        }
        
        override init(frame: CGRect) {
            super.init(frame: frame)
            
            backgroundColor = .secondaryBraveBackground
            accessibilityLabel = Strings.tabTrayAccessibilityLabel
            
            let buttonsStackView = UIStackView().then {
                $0.distribution = .equalSpacing
                $0.addStackViewItems(
                    .view(privateModeButton),
                    .view(newTabButton),
                    .view(doneButton)
                )
                $0.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)
                $0.layoutMargins = UIEdgeInsets(top: 0, left: 16, bottom: 0, right: 16)
                $0.isLayoutMarginsRelativeArrangement = true
            }
            
            let stackView = UIStackView().then {
                $0.axis = .vertical
                $0.spacing = 0
                $0.addStackViewItems(
                    .view(privateModeInfo),
                    .view(collectionView),
                    .customSpace(4),
                    .view(buttonsStackView))
                $0.isAccessibilityElement = false
            }
            
            privateModeButton.snp.makeConstraints {
                $0.width.equalTo(doneButton.snp.width)
            }
            
            addSubview(stackView)
            stackView.snp.makeConstraints {
                $0.edges.equalTo(self.safeArea.edges).inset(8)
            }
            
            privateModeInfo.isHidden = true
        }
        
        var numberOfColumns: Int {
            let compactWidthRegularHeight = traitCollection.horizontalSizeClass == .compact &&
                traitCollection.verticalSizeClass == .regular
            return compactWidthRegularHeight ? 2 : 3
        }
        
        @available(*, unavailable)
        required init(coder: NSCoder) { fatalError() }
        
        override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
            super.traitCollectionDidChange(previousTraitCollection)
            
            if previousTraitCollection != traitCollection {
                let regularWidthHeight = traitCollection.horizontalSizeClass == .regular &&
                    traitCollection.verticalSizeClass == .regular
                
                let cellHeight = regularWidthHeight ? UX.largeCellHeight : UX.regularCellHeight
                
                let layout = generateLayout(numberOfColumns: numberOfColumns, cellHeight: cellHeight)
                
                // FIXME: with `animated: true` it glitches out.
                collectionView.setCollectionViewLayout(layout, animated: false)
            }
        }
        
        @objc func privateModeLearnMoreAction() {
            privateModeInfo.detailsLabel.alpha = 0.0
            UIView.animate(withDuration: 0.15, animations: {
                self.privateModeInfo.learnMoreButton.isHidden = true
                self.privateModeInfo.detailsLabel.isHidden = false
            }, completion: { _ in
                    UIView.animate(withDuration: 0.15) {
                        self.privateModeInfo.detailsLabel.alpha = 1.0
                    }
            })
            // Needs to be done in a separate call so the stack view's height is updated properly based on the hidden
            // status of learnMoreButton/detailsLabel
            UIView.animate(withDuration: 0.2) {
                self.privateModeInfo.updateContentInset()
            }
        }
        
        func showPrivateModeInfo() {
            // Private mode always uses dark style.
            // Setting it explicitly here makes a smoother transition.
            overrideUserInterfaceStyle = .dark
            privateModeInfo.isHidden = false
            collectionView.isHidden = true
        }
        
        func hidePrivateModeInfo() {
            overrideUserInterfaceStyle = .unspecified
            privateModeInfo.isHidden = true
            collectionView.isHidden = false
        }
        
        var isPrivateModeInfoShowing: Bool {
            !privateModeInfo.isHidden
        }
    }
}
