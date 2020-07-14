// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import Shared

/// Additonal information related to an action performed on a feed item
struct FeedItemActionContext {
    /// The feed item actioned upon
    var item: FeedItem
    /// The card that this item is displayed in
    var card: FeedCard
    /// The index path of the card in the collection view
    var indexPath: IndexPath
}

typealias FeedItemActionHandler = (FeedItemAction, _ context: FeedItemActionContext) -> Void

class BraveTodaySectionProvider: NSObject, NTPObservableSectionProvider {
    let dataSource: FeedDataSource
    var sectionDidChange: (() -> Void)?
    var actionHandler: FeedItemActionHandler
    
    init(dataSource: FeedDataSource, actionHandler: @escaping FeedItemActionHandler) {
        self.dataSource = dataSource
        self.actionHandler = actionHandler
        
        super.init()
        
        self.dataSource.load { [weak self] in
            self?.sectionDidChange?()
        }
    }
    
    @objc private func tappedBraveTodaySettings() {
        
    }
    
    func registerCells(to collectionView: UICollectionView) {
        collectionView.register(FeedCardCell<BraveTodayWelcomeView>.self)
        collectionView.register(FeedCardCell<HeadlineCardView>.self)
        collectionView.register(FeedCardCell<SmallHeadlinePairCardView>.self)
        collectionView.register(FeedCardCell<VerticalFeedGroupView>.self)
        collectionView.register(FeedCardCell<HorizontalFeedGroupView>.self)
        collectionView.register(FeedCardCell<NumberedFeedGroupView>.self)
        collectionView.register(FeedCardCell<SponsorCardView>.self)
    }
    
    var landscapeBehavior: NTPLandscapeSizingBehavior {
        .fullWidth
    }
    
    func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
        return dataSource.cards.count + 1
    }
    
    func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
        var size = fittingSizeForCollectionView(collectionView, section: indexPath.section)
        if indexPath.item == 0 {
            size.height = 300
        } else if let card = dataSource.cards[safe: indexPath.item - 1] {
            size.height = card.estimatedHeight(for: size.width)
        }
        return size
    }
    
    func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, insetForSectionAt section: Int) -> UIEdgeInsets {
        return UIEdgeInsets(top: 0, left: 16, bottom: 16, right: 16)
    }
    
    func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, minimumLineSpacingForSectionAt section: Int) -> CGFloat {
        return 20
    }
    
    func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
        if indexPath.item == 0 {
            return collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<BraveTodayWelcomeView>
        }
        
        guard let card = dataSource.cards[safe: indexPath.item - 1] else {
            assertionFailure()
            return UICollectionViewCell()
        }
        
        func handler(for item: FeedItem) -> (Int, FeedItemAction) -> Void {
            return { [weak self] _, action in
                self?.actionHandler(action, .init(item: item, card: card, indexPath: indexPath))
            }
        }
        func handler(from feedList: @escaping (Int) -> FeedItem) -> (Int, FeedItemAction) -> Void {
            return { [weak self] index, action in
                self?.actionHandler(action, .init(item: feedList(index), card: card, indexPath: indexPath))
            }
        }
        
        func contextMenu(from feedList: @escaping (Int) -> FeedItem) -> FeedItemMenu {
            if #available(iOS 13.0, *) {
                return .init { index -> UIMenu? in
                    let item = feedList(index)
                    let context = FeedItemActionContext(item: item, card: card, indexPath: indexPath)
                    
                    var openInNewTab: UIAction {
                        .init(title: Strings.openNewTabButtonTitle, handler: UIAction.deferredActionHandler { _ in
                            self.actionHandler(.opened(inNewTab: true), context)
                        })
                    }
                    
                    var openInNewPrivateTab: UIAction {
                        .init(title: Strings.openNewPrivateTabButtonTitle, handler: UIAction.deferredActionHandler { _ in
                            self.actionHandler(.opened(inNewTab: true, switchingToPrivateMode: true), context)
                        })
                    }
                    
                    var hideContent: UIAction {
                        // FIXME: Localize, Get our own image
                        .init(title: "Hide Content", image: UIImage(systemName: "eye.slash.fill"), handler: UIAction.deferredActionHandler { _ in
                            self.actionHandler(.hide, context)
                        })
                    }
                    
                    var blockSource: UIAction {
                        // FIXME: Localize, Get our own image
                        .init(title: "Block Source", image: UIImage(systemName: "nosign"), attributes: .destructive, handler: UIAction.deferredActionHandler { _ in
                            self.actionHandler(.blockSource, context)
                        })
                    }
                    
                    let openActions: [UIAction] = [
                        openInNewTab,
                        // Brave Today is only available in normal tabs, so this isn't technically required
                        // but good to be on the safe side
                        !PrivateBrowsingManager.shared.isPrivateBrowsing ?
                            openInNewPrivateTab :
                        nil
                        ].compactMap({ $0 })
                    let manageActions = [
                        hideContent,
                        blockSource
                    ]
                    
                    return UIMenu(title: item.content.title, children: [
                        UIMenu(title: "", options: [.displayInline], children: openActions),
                        UIMenu(title: "", options: [.displayInline], children: manageActions)
                    ])
                }
            }
            return .init { index -> FeedItemMenu.LegacyContext? in
                let item = feedList(index)
                let context = FeedItemActionContext(item: item, card: card, indexPath: indexPath)
                
                var openInNewTab: UIAlertAction {
                    .init(title: Strings.openNewTabButtonTitle, style: .default, handler: { _ in
                        self.actionHandler(.opened(inNewTab: true), context)
                    })
                }
                
                var openInNewPrivateTab: UIAlertAction {
                    .init(title: Strings.openNewPrivateTabButtonTitle, style: .default, handler: { _ in
                        self.actionHandler(.opened(inNewTab: true, switchingToPrivateMode: true), context)
                    })
                }
                
                var hideContent: UIAlertAction {
                    // FIXME: Localize
                    .init(title: "Hide Content", style: .default, handler: { _ in
                        self.actionHandler(.hide, context)
                    })
                }
                
                var blockSource: UIAlertAction {
                    // FIXME: Localize
                    .init(title: "Block Source", style: .destructive, handler: { _ in
                        self.actionHandler(.blockSource, context)
                    })
                }
                
                let cancel = UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil)
                
                return .init(
                    title: item.content.title,
                    message: nil,
                    actions: [
                        openInNewTab,
                        // Brave Today is only available in normal tabs, so this isn't technically required
                        // but good to be on the safe side
                        !PrivateBrowsingManager.shared.isPrivateBrowsing ?
                            openInNewPrivateTab :
                        nil,
                        hideContent,
                        blockSource,
                        cancel
                    ].compactMap { $0 }
                )
            }
        }
        
        func contextMenu(for item: FeedItem) -> FeedItemMenu {
            return contextMenu(from: { _  in item })
        }
        
        switch card {
        case .sponsor(let item):
            let cell = collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<SponsorCardView>
            cell.content.feedView.setupWithItem(item, brandVisibility: .none)
            cell.content.actionHandler = handler(for: item)
            cell.content.contextMenu = contextMenu(for: item)
            return cell
        case .headline(let item):
            let cell = collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<HeadlineCardView>
            cell.content.feedView.setupWithItem(item, brandVisibility: .logo)
            cell.content.actionHandler = handler(for: item)
            cell.content.contextMenu = contextMenu(for: item)
            return cell
        case .headlinePair(let pair):
            let cell = collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<SmallHeadlinePairCardView>
            cell.content.smallHeadelineCardViews.left.feedView.setupWithItem(pair.first, brandVisibility: .logo)
            cell.content.smallHeadelineCardViews.right.feedView.setupWithItem(pair.second, brandVisibility: .logo)
            cell.content.actionHandler = handler(from: { $0 == 0 ? pair.first : pair.second })
            cell.content.contextMenu = contextMenu(from: { $0 == 0 ? pair.first : pair.second })
            return cell
        case .group(let items, let title, let direction, let displayBrand):
            let groupView: FeedGroupView
            let cell: UICollectionViewCell
            switch direction {
            case .horizontal:
                let horizontalCell = collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<HorizontalFeedGroupView>
                groupView = horizontalCell.content
                cell = horizontalCell
            case .vertical:
                let verticalCell = collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<VerticalFeedGroupView>
                groupView = verticalCell.content
                cell = verticalCell
            @unknown default:
                assertionFailure()
                return UICollectionViewCell()
            }
            groupView.titleLabel.text = title
            groupView.titleLabel.isHidden = title.isEmpty
            
            let isItemsAllSameSource = Set(items.map(\.content.publisherID)).count == 1
            
            zip(groupView.feedViews, items).forEach { (view, item) in
                view.setupWithItem(
                    item,
                    brandVisibility: (isItemsAllSameSource && displayBrand) ? .none : .name
                )
            }
            if displayBrand {
                if let logo = items.first?.source.logo {
                    groupView.groupBrandImageView.sd_setImage(with: logo, placeholderImage: nil, options: .avoidAutoSetImage) { (image, _, cacheType, _) in
                        if cacheType == .none {
                            UIView.transition(
                                with: groupView.groupBrandImageView,
                                duration: 0.35,
                                options: [.transitionCrossDissolve, .curveEaseInOut],
                                animations: {
                                    groupView.groupBrandImageView.image = image
                            }
                            )
                        } else {
                            groupView.groupBrandImageView.image = image
                        }
                    }
                }
            } else {
                groupView.groupBrandImageView.image = nil
            }
            groupView.groupBrandImageView.isHidden = !displayBrand
            groupView.actionHandler = handler(from: { items[$0] })
            groupView.contextMenu = contextMenu(from: { items[$0] })
            return cell
        case .numbered(let items, let title):
            let cell = collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<NumberedFeedGroupView>
            cell.content.titleLabel.text = title
            zip(cell.content.feedViews, items).forEach { (view, item) in
                view.setupWithItem(item, brandVisibility: .none)
            }
            cell.content.actionHandler = handler(from: { items[$0] })
            cell.content.contextMenu = contextMenu(from: { items[$0] })
            return cell
        }
    }
}

extension FeedItemView {
    enum BrandVisibility {
        case none
        case name
        case logo
    }
    
    func setupWithItem(_ feedItem: FeedItem, brandVisibility: BrandVisibility = .none) {
        isContentHidden = feedItem.isContentHidden
        titleLabel.text = feedItem.content.title
        if #available(iOS 13, *) {
            dateLabel.text = RelativeDateTimeFormatter().localizedString(for: feedItem.content.publishTime, relativeTo: Date())
        }
        if feedItem.content.imageURL == nil {
            thumbnailImageView.isHidden = true
        } else {
            thumbnailImageView.isHidden = false
            thumbnailImageView.sd_setImage(with: feedItem.content.imageURL, placeholderImage: nil, options: .avoidAutoSetImage, completed: { (image, _, cacheType, _) in
                if cacheType == .none {
                    UIView.transition(
                        with: self.thumbnailImageView,
                        duration: 0.35,
                        options: [.transitionCrossDissolve, .curveEaseInOut],
                        animations: {
                            self.thumbnailImageView.image = image
                    }
                    )
                } else {
                    self.thumbnailImageView.image = image
                }
            })
        }
        brandLabelView.text = nil
        brandImageView.image = nil
        switch brandVisibility {
        case .none:
            break
        case .name:
            brandLabelView.text = feedItem.content.publisherName
        case .logo:
            if let logo = feedItem.source.logo {
                brandImageView.sd_setImage(with: logo, placeholderImage: nil, options: .avoidAutoSetImage) { (image, _, cacheType, _) in
                    if cacheType == .none {
                        UIView.transition(
                            with: self.brandImageView,
                            duration: 0.35,
                            options: [.transitionCrossDissolve, .curveEaseInOut],
                            animations: {
                                self.brandImageView.image = image
                            }
                        )
                    } else {
                        self.brandImageView.image = image
                    }
                }
            }
        }
    }
}
