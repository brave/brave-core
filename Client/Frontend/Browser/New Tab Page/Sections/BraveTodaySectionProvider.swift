// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import Shared
import BraveShared

/// Additonal information related to an action performed on a feed item
struct FeedItemActionContext {
    /// The feed item actioned upon
    var item: FeedItem
    /// The card that this item is displayed in
    var card: FeedCard
    /// The index path of the card in the collection view
    var indexPath: IndexPath
}

/// The section provider for Brave Today. 
class BraveTodaySectionProvider: NSObject, NTPObservableSectionProvider {
    /// Set of actions that can occur from the Brave Today section
    enum Action {
        /// The user interacted with the welcome card
        case welcomeCardAction(WelcomeCardAction)
        /// The user tapped sources & settings on the empty card
        case emptyCardTappedSourcesAndSettings
        /// The user tapped refresh on the error card
        case errorCardTappedRefresh
        /// The user tapped to show more brave offers on one of the deal cards
        case moreBraveOffersTapped
        /// The user performed an action on a feed item
        case itemAction(FeedItemAction, context: FeedItemActionContext)
    }
    
    let dataSource: FeedDataSource
    var sectionDidChange: (() -> Void)?
    var actionHandler: (Action) -> Void
    
    init(dataSource: FeedDataSource,
         actionHandler: @escaping (Action) -> Void) {
        self.dataSource = dataSource
        self.actionHandler = actionHandler
        
        super.init()
    }
    
    func registerCells(to collectionView: UICollectionView) {
        collectionView.register(FeedCardCell<BraveTodayWelcomeView>.self)
        collectionView.register(FeedCardCell<BraveTodayErrorView>.self)
        collectionView.register(FeedCardCell<BraveTodayEmptyFeedView>.self)
        collectionView.register(FeedCardCell<HeadlineCardView>.self)
        collectionView.register(FeedCardCell<SmallHeadlinePairCardView>.self)
        collectionView.register(FeedCardCell<VerticalFeedGroupView>.self)
        collectionView.register(FeedCardCell<HorizontalFeedGroupView>.self)
        collectionView.register(FeedCardCell<DealsFeedGroupView>.self)
        collectionView.register(FeedCardCell<NumberedFeedGroupView>.self)
        collectionView.register(FeedCardCell<SponsorCardView>.self)
    }
    
    var landscapeBehavior: NTPLandscapeSizingBehavior {
        .fullWidth
    }
    
    private var isShowingIntroCard: Bool {
        Preferences.BraveToday.isShowingIntroCard.value
    }
    
    func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
        if !Preferences.BraveToday.isEnabled.value {
            return 0
        }
        switch dataSource.state {
        case .failure:
            return 1
        case .initial, .loading:
            return 0
        case .success(let cards):
            if cards.isEmpty {
                return 1
            }
            return cards.count + (isShowingIntroCard ? 1 : 0)
        }
    }
    
    func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
        var size = fittingSizeForCollectionView(collectionView, section: indexPath.section)
        switch dataSource.state {
        case .failure, .initial, .loading:
            size.height = 300
        case .success(let cards):
            if cards.isEmpty {
                size.height = 300
            } else {
                size.height = cards[safe: indexPath.item - (isShowingIntroCard ? 1 : 0)]?.estimatedHeight(for: size.width) ?? 300
            }
        }
        return size
    }
    
    func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, insetForSectionAt section: Int) -> UIEdgeInsets {
        return UIEdgeInsets(top: 0, left: 16, bottom: 16, right: 16)
    }
    
    func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, minimumLineSpacingForSectionAt section: Int) -> CGFloat {
        return 20
    }
    
    func collectionView(_ collectionView: UICollectionView, willDisplay cell: UICollectionViewCell, forItemAt indexPath: IndexPath) {
        if indexPath.item == 0, let cell = cell as? FeedCardCell<BraveTodayWelcomeView> {
            cell.content.graphicAnimationView.play()
        }
    }
    
    func collectionView(_ collectionView: UICollectionView, didEndDisplaying cell: UICollectionViewCell, forItemAt indexPath: IndexPath) {
        if indexPath.item == 0, let cell = cell as? FeedCardCell<BraveTodayWelcomeView> {
            cell.content.graphicAnimationView.stop()
        }
    }
    
    func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
        if let error = dataSource.state.error {
            let cell = collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<BraveTodayErrorView>
            if let urlError = error as? URLError, urlError.code == .notConnectedToInternet {
                cell.content.titleLabel.text = Strings.BraveToday.errorNoInternetTitle
                cell.content.errorMessageLabel.text = Strings.BraveToday.errorNoInternetBody
            } else {
                cell.content.titleLabel.text = Strings.BraveToday.errorGeneralTitle
                cell.content.errorMessageLabel.text = Strings.BraveToday.errorGeneralBody
            }
            if case .loading = dataSource.state {
                cell.content.refreshButton.isLoading = true
            } else {
                cell.content.refreshButton.isLoading = false
            }
            cell.content.refreshButtonTapped = { [weak self] in
                if !cell.content.refreshButton.isLoading {
                    cell.content.refreshButton.isLoading = true
                    self?.actionHandler(.errorCardTappedRefresh)
                }
            }
            return cell
        }
        
        if let cards = dataSource.state.cards, cards.isEmpty {
            let cell = collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<BraveTodayEmptyFeedView>
            cell.content.sourcesAndSettingsButtonTapped = { [weak self] in
                self?.actionHandler(.emptyCardTappedSourcesAndSettings)
            }
            return cell
        }
        
        if isShowingIntroCard && indexPath.item == 0 {
            let cell = collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<BraveTodayWelcomeView>
            cell.content.introCardActionHandler = { [weak self] action in
                self?.actionHandler(.welcomeCardAction(action))
            }
            return cell
        }
        
        let indexDisplacement = isShowingIntroCard ? 1 : 0
        guard let card = dataSource.state.cards?[safe: indexPath.item - indexDisplacement] else {
            assertionFailure()
            return UICollectionViewCell()
        }
        
        switch card {
        case .sponsor(let item):
            let cell = collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<SponsorCardView>
            cell.content.feedView.setupWithItem(item)
            cell.content.actionHandler = handler(for: item, card: card, indexPath: indexPath)
            cell.content.contextMenu = contextMenu(for: item, card: card, indexPath: indexPath)
            return cell
        case .deals(let items, let title):
            let cell = collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<DealsFeedGroupView>
            cell.content.titleLabel.text = title
            cell.content.titleLabel.isHidden = title.isEmpty
            zip(cell.content.feedViews, items.indices).forEach { (view, index) in
                let item = items[index]
                view.setupWithItem(
                    item,
                    isBrandVisible: false
                )
                view.descriptionLabel.text = item.content.description
                // Force thumbnail to show up for deals even if the data source has none for UI purposes
                view.thumbnailImageView.isHidden = false
            }
            cell.content.moreOffersButtonTapped = { [weak self] in
                self?.actionHandler(.moreBraveOffersTapped)
            }
            cell.content.actionHandler = handler(from: { items[$0] }, card: card, indexPath: indexPath)
            cell.content.contextMenu = contextMenu(from: { items[$0] }, card: card, indexPath: indexPath)
            return cell
        case .headline(let item):
            let cell = collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<HeadlineCardView>
            cell.content.feedView.setupWithItem(item)
            cell.content.actionHandler = handler(for: item, card: card, indexPath: indexPath)
            cell.content.contextMenu = contextMenu(for: item, card: card, indexPath: indexPath)
            return cell
        case .headlinePair(let pair):
            let cell = collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<SmallHeadlinePairCardView>
            cell.content.smallHeadelineCardViews.left.feedView.setupWithItem(pair.first)
            cell.content.smallHeadelineCardViews.right.feedView.setupWithItem(pair.second)
            cell.content.actionHandler = handler(from: { $0 == 0 ? pair.first : pair.second }, card: card, indexPath: indexPath)
            cell.content.contextMenu = contextMenu(from: { $0 == 0 ? pair.first : pair.second }, card: card, indexPath: indexPath)
            return cell
        case .group(let items, let title, let direction, _):
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
            
            zip(groupView.feedViews, items.indices).forEach { (view, index) in
                let item = items[index]
                view.setupWithItem(item)
            }
            groupView.actionHandler = handler(from: { items[$0] }, card: card, indexPath: indexPath)
            groupView.contextMenu = contextMenu(from: { items[$0] }, card: card, indexPath: indexPath)
            return cell
        case .numbered(let items, let title):
            let cell = collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<NumberedFeedGroupView>
            cell.content.titleLabel.text = title
            zip(cell.content.feedViews, items.indices).forEach { (view, index) in
                let item = items[index]
                view.setupWithItem(item)
            }
            cell.content.actionHandler = handler(from: { items[$0] }, card: card, indexPath: indexPath)
            cell.content.contextMenu = contextMenu(from: { items[$0] }, card: card, indexPath: indexPath)
            return cell
        }
    }
    
    private func handler(from feedList: @escaping (Int) -> FeedItem, card: FeedCard, indexPath: IndexPath) -> (Int, FeedItemAction) -> Void {
        return { [weak self] index, action in
            self?.actionHandler(.itemAction(action, context: .init(item: feedList(index), card: card, indexPath: indexPath)))
        }
    }
    
    private func handler(for item: FeedItem, card: FeedCard, indexPath: IndexPath) -> (Int, FeedItemAction) -> Void {
        return handler(from: { _ in item }, card: card, indexPath: indexPath)
    }
    
    private func contextMenu(from feedList: @escaping (Int) -> FeedItem, card: FeedCard, indexPath: IndexPath) -> FeedItemMenu {
        typealias MenuActionHandler = (_ context: FeedItemActionContext) -> Void
        
        func itemActionHandler(_ action: FeedItemAction, _ context: FeedItemActionContext) {
            self.actionHandler(.itemAction(action, context: context))
        }
        
        let openInNewTabHandler: MenuActionHandler = { context in
            itemActionHandler(.opened(inNewTab: true), context)
        }
        let openInNewPrivateTabHandler: MenuActionHandler = { context in
            itemActionHandler(.opened(inNewTab: true, switchingToPrivateMode: true), context)
        }
        let toggleSourceHandler: MenuActionHandler = { context in
            itemActionHandler(.toggledSource, context)
        }
        
        if #available(iOS 13.0, *) {
            return .init { [weak self] index -> UIMenu? in
                guard let self = self else { return nil }
                let item = feedList(index)
                let context = FeedItemActionContext(item: item, card: card, indexPath: indexPath)
                
                func mapDeferredHandler(_ handler: @escaping MenuActionHandler) -> UIActionHandler {
                    return UIAction.deferredActionHandler { _ in
                        handler(context)
                    }
                }
                
                var openInNewTab: UIAction {
                    .init(title: Strings.openNewTabButtonTitle, image: UIImage(named: "brave.plus"), handler: mapDeferredHandler(openInNewTabHandler))
                }
                
                var openInNewPrivateTab: UIAction {
                    .init(title: Strings.openNewPrivateTabButtonTitle, image: UIImage(named: "brave.shades"), handler: mapDeferredHandler(openInNewPrivateTabHandler))
                }
                
                var disableSource: UIAction {
                    .init(title: String(format: Strings.BraveToday.disablePublisherContent, item.source.name), image: UIImage(named: "disable.feed.source"), attributes: .destructive, handler: mapDeferredHandler(toggleSourceHandler))
                }
                
                var enableSource: UIAction {
                    .init(title: String(format: Strings.BraveToday.enablePublisherContent, item.source.name), image: UIImage(named: "enable.feed.source"), handler: mapDeferredHandler(toggleSourceHandler))
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
                    self.dataSource.isSourceEnabled(item.source) ? disableSource : enableSource
                ]
                
                var children: [UIMenu] = [
                    UIMenu(title: "", options: [.displayInline], children: openActions),
                ]
                if context.item.content.contentType != .sponsor {
                    children.append(UIMenu(title: "", options: [.displayInline], children: manageActions))
                }
                return UIMenu(title: item.content.url?.absoluteString ?? "", children: children)
            }
        }
        return .init { [weak self] index -> FeedItemMenu.LegacyContext? in
            guard let self = self else { return nil }
            let item = feedList(index)
            let context = FeedItemActionContext(item: item, card: card, indexPath: indexPath)
            
            func mapHandler(_ handler: @escaping MenuActionHandler) -> UIAlertActionCallback {
                return { _ in
                    handler(context)
                }
            }
            
            var openInNewTab: UIAlertAction {
                .init(title: Strings.openNewTabButtonTitle, style: .default, handler: mapHandler(openInNewTabHandler))
            }
            
            var openInNewPrivateTab: UIAlertAction {
                .init(title: Strings.openNewPrivateTabButtonTitle, style: .default, handler: mapHandler(openInNewPrivateTabHandler))
            }
            
            var disableSource: UIAlertAction {
                .init(title: String(format: Strings.BraveToday.disablePublisherContent, item.source.name), style: .destructive, handler: mapHandler(toggleSourceHandler))
            }
            
            var enableSource: UIAlertAction {
                .init(title: String(format: Strings.BraveToday.enablePublisherContent, item.source.name), style: .default, handler: mapHandler(toggleSourceHandler))
            }
            
            let cancel = UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil)
            
            var actions: [UIAlertAction?] = [
                openInNewTab,
                // Brave Today is only available in normal tabs, so this isn't technically required
                // but good to be on the safe side
                !PrivateBrowsingManager.shared.isPrivateBrowsing ?
                    openInNewPrivateTab :
                nil
            ]
            
            if context.item.content.contentType != .sponsor {
                if self.dataSource.isSourceEnabled(context.item.source) {
                    actions.append(disableSource)
                } else {
                    actions.append(enableSource)
                }
            }
            
            actions.append(cancel)
            
            return .init(
                title: item.content.url?.absoluteString,
                message: nil,
                actions: actions.compactMap { $0 }
            )
        }
    }
    
    private func contextMenu(for item: FeedItem, card: FeedCard, indexPath: IndexPath) -> FeedItemMenu {
        return contextMenu(from: { _  in item }, card: card, indexPath: indexPath)
    }
}

extension FeedItemView {
    func setupWithItem(_ feedItem: FeedItem, isBrandVisible: Bool = true) {
        titleLabel.text = feedItem.content.title
        if #available(iOS 13, *) {
            dateLabel.text = RelativeDateTimeFormatter().localizedString(
                for: feedItem.content.publishTime,
                relativeTo: Date()
            )
        } else {
            dateLabel.text = LegacyRelativeDateTimeFormatter().localizedString(
                for: feedItem.content.publishTime,
                relativeTo: Date()
            )
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
        
        brandContainerView.textLabel.text = nil
        if isBrandVisible {
            brandContainerView.textLabel.text = feedItem.source.name
        }
    }
}
