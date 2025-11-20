// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveNews
import BraveUI
import DesignSystem
import Foundation
import Growth
import Preferences
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

/// The section provider for Brave News.
class BraveNewsSectionProvider: NSObject, NTPObservableSectionProvider {
  /// Set of actions that can occur from the Brave News section
  enum Action {
    /// The user interacted with the welcome card
    case optInCardAction(OptInCardAction)
    /// The user tapped sources & settings on the empty card
    case emptyCardTappedSourcesAndSettings
    /// The user tapped refresh on the error card
    case errorCardTappedRefresh
    /// The user tapped to show more brave offers on one of the deal cards
    case moreBraveOffersTapped
    /// The user performed an action on a feed item
    case itemAction(FeedItemAction, context: FeedItemActionContext)
    /// The user performed an action on an Rate brave card
    case rateCardAction(RatingCardAction)
  }

  let dataSource: FeedDataSource
  let rewards: BraveRewards
  var sectionDidChange: (() -> Void)?
  var actionHandler: (Action) -> Void

  private var viewedCards: Set<FeedCard> = []

  init(
    dataSource: FeedDataSource,
    rewards: BraveRewards,
    actionHandler: @escaping (Action) -> Void
  ) {
    self.dataSource = dataSource
    self.rewards = rewards
    self.actionHandler = actionHandler

    super.init()

    self.recordWeeklyAdsViewedP3A(adViewed: false)
    self.recordWeeklyCardsViewedP3A(cardViewed: false)
  }

  func registerCells(to collectionView: UICollectionView) {
    collectionView.register(FeedCardCell<BraveNewsOptInView>.self)
    collectionView.register(FeedCardCell<BraveNewsErrorView>.self)
    collectionView.register(FeedCardCell<BraveNewsEmptyFeedView>.self)
    collectionView.register(FeedCardCell<HeadlineCardView>.self)
    collectionView.register(FeedCardCell<SmallHeadlinePairCardView>.self)
    collectionView.register(FeedCardCell<SmallHeadlineRatePairCardView>.self)
    collectionView.register(FeedCardCell<VerticalFeedGroupView>.self)
    collectionView.register(FeedCardCell<HorizontalFeedGroupView>.self)
    collectionView.register(FeedCardCell<DealsFeedGroupView>.self)
    collectionView.register(FeedCardCell<NumberedFeedGroupView>.self)
    collectionView.register(FeedCardCell<SponsorCardView>.self)
  }

  var landscapeBehavior: NTPLandscapeSizingBehavior {
    .fullWidth
  }

  private var isShowingOptInCard: Bool {
    Preferences.BraveNews.isShowingOptIn.value
  }

  func collectionView(
    _ collectionView: UICollectionView,
    numberOfItemsInSection section: Int
  ) -> Int {
    if isShowingOptInCard {
      return 1
    }
    if !Preferences.BraveNews.isEnabled.value {
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
      return cards.count
    }
  }

  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    sizeForItemAt indexPath: IndexPath
  ) -> CGSize {
    var size = fittingSizeForCollectionView(collectionView, section: indexPath.section)
    switch dataSource.state {
    case .failure, .initial, .loading:
      size.height = 300
    case .success(let cards):
      if cards.isEmpty {
        size.height = 300
      } else {
        size.height = cards[safe: indexPath.item]?.estimatedHeight(for: size.width) ?? 300
      }
    }
    return size
  }

  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    insetForSectionAt section: Int
  ) -> UIEdgeInsets {
    return UIEdgeInsets(top: 0, left: 16, bottom: 16, right: 16)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    minimumLineSpacingForSectionAt section: Int
  ) -> CGFloat {
    return 20
  }

  private var iabTrackedCellContexts: [IndexPath: ViewportTrackedCardContext] = [:]

  /// Information about a IAB tracked card to determine if a user viewed the ad by scrolling
  /// at least 50% of the cell into the viewport for at least 1 second
  private class ViewportTrackedCardContext {
    var collectionView: UICollectionView
    var action: () -> Void
    var runningTimer: Timer?

    deinit {
      runningTimer?.invalidate()
    }

    init(collectionView: UICollectionView, action: @escaping () -> Void) {
      self.collectionView = collectionView
      self.action = action
    }
  }

  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    func cellAtIndexPathIsMostlyVisible(
      _ indexPath: IndexPath,
      context: ViewportTrackedCardContext
    ) -> Bool {
      if let cell = context.collectionView.cellForItem(at: indexPath) {
        if cell.frame.intersection(context.collectionView.bounds).height > cell.bounds.height / 2.0
        {
          return true
        }
      }
      return false
    }
    if iabTrackedCellContexts.isEmpty { return }
    for (indexPath, context) in iabTrackedCellContexts {
      if cellAtIndexPathIsMostlyVisible(indexPath, context: context),
        context.runningTimer == nil
      {
        context.runningTimer = Timer.scheduledTimer(
          withTimeInterval: 1.0,
          repeats: false,
          block: { [weak self] timer in
            guard let self = self else { return }
            if let context = self.iabTrackedCellContexts[indexPath],
              context.runningTimer == timer,
              cellAtIndexPathIsMostlyVisible(indexPath, context: context)
            {
              // Still at least 50% visible
              context.action()
            }
          }
        )
      }
    }
  }

  func collectionView(
    _ collectionView: UICollectionView,
    willDisplay cell: UICollectionViewCell,
    forItemAt indexPath: IndexPath
  ) {
    if indexPath.item == 0, let cell = cell as? FeedCardCell<BraveNewsOptInView> {
      DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
        // Seems like there is a CoreGraphics glitch somewhere here after we create a new tab it starts
        // playing too quickly before the tab tray animation is done, so it doesn't play... possibly a Lottie
        // bug
        cell.content.graphicAnimationView.play()
      }
    }
    if let card = dataSource.state.cards?[safe: indexPath.item] {
      if !viewedCards.contains(card) && collectionView.contentOffset.y > 0 {
        if indexPath.item == 1, let firstCard = dataSource.state.cards?.first,
          !viewedCards.contains(firstCard), card != firstCard
        {
          // Since we don't record the peeking card we want to make sure that it counts once its in view
          viewedCards.insert(firstCard)
          recordWeeklyCardsViewedP3A(cardViewed: true)
        }
        viewedCards.insert(card)
        recordWeeklyCardsViewedP3A(cardViewed: true)
        recordWeeklyMaxRowsViewedP3A()
      }
      if case .headlineRatingCardPair = card {
        // The rating card is presented
        Preferences.Review.newsCardShownDate.value = Date()
      }
    }
  }

  func collectionView(
    _ collectionView: UICollectionView,
    didEndDisplaying cell: UICollectionViewCell,
    forItemAt indexPath: IndexPath
  ) {
    // Due to an iOS bug, didEndDisplaying can be called for a cell which is
    // still in a collection view. In this case didEndDisplaying should be
    // ignored.
    if collectionView.cellForItem(at: indexPath) == nil {
      iabTrackedCellContexts[indexPath] = nil
    }
    if indexPath.item == 0, let cell = cell as? FeedCardCell<BraveNewsOptInView> {
      cell.content.graphicAnimationView.stop()
    }
  }

  func collectionView(
    _ collectionView: UICollectionView,
    cellForItemAt indexPath: IndexPath
  ) -> UICollectionViewCell {
    if let error = dataSource.state.error {
      let cell =
        collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<BraveNewsErrorView>
      if let urlError = error as? URLError, urlError.code == .notConnectedToInternet {
        cell.content.titleLabel.text = Strings.BraveNews.errorNoInternetTitle
        cell.content.errorMessageLabel.text = Strings.BraveNews.errorNoInternetBody
      } else {
        cell.content.titleLabel.text = Strings.BraveNews.errorGeneralTitle
        cell.content.errorMessageLabel.text = Strings.BraveNews.errorGeneralBody
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

    if isShowingOptInCard && indexPath.item == 0 {
      let cell =
        collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<BraveNewsOptInView>
      cell.content.optInCardActionHandler = { [weak self] action in
        if action == .turnOnBraveNewsButtonTapped && !cell.content.turnOnBraveNewsButton.isLoading {
          cell.content.turnOnBraveNewsButton.isLoading = true
        }
        self?.actionHandler(.optInCardAction(action))
      }
      return cell
    }

    if let cards = dataSource.state.cards, cards.isEmpty {
      let cell =
        collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<BraveNewsEmptyFeedView>
      cell.content.sourcesAndSettingsButtonTapped = { [weak self] in
        self?.actionHandler(.emptyCardTappedSourcesAndSettings)
      }
      return cell
    }

    guard let card = dataSource.state.cards?[safe: indexPath.item] else {
      assertionFailure()
      return collectionView.dequeueReusableCell(for: indexPath)
        as FeedCardCell<BraveNewsEmptyFeedView>
    }

    switch card {
    case .sponsor(let item):
      let cell = collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<SponsorCardView>
      cell.content.feedView.setupWithItem(item)
      cell.content.actionHandler = handler(for: item, card: card, indexPath: indexPath)
      cell.content.contextMenu = contextMenu(for: item, card: card, indexPath: indexPath)
      return cell
    case .deals(let items, let title):
      let cell =
        collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<DealsFeedGroupView>
      let title = title ?? Strings.BraveNews.deals
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
      let cell =
        collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<HeadlineCardView>
      cell.content.feedView.setupWithItem(item)
      cell.content.actionHandler = handler(for: item, card: card, indexPath: indexPath)
      cell.content.contextMenu = contextMenu(for: item, card: card, indexPath: indexPath)
      return cell
    case .headlinePair(let pair):
      let cell =
        collectionView.dequeueReusableCell(for: indexPath)
        as FeedCardCell<SmallHeadlinePairCardView>
      cell.content.smallHeadelineCardViews.left.feedView.setupWithItem(pair.first)
      cell.content.smallHeadelineCardViews.right.feedView.setupWithItem(pair.second)
      cell.content.actionHandler = handler(
        from: { $0 == 0 ? pair.first : pair.second },
        card: card,
        indexPath: indexPath
      )
      cell.content.contextMenu = contextMenu(
        from: { $0 == 0 ? pair.first : pair.second },
        card: card,
        indexPath: indexPath
      )
      return cell
    case .headlineRatingCardPair(let item):
      let cell =
        collectionView.dequeueReusableCell(for: indexPath)
        as FeedCardCell<SmallHeadlineRatePairCardView>
      cell.content.smallHeadlineRateCardViews.smallHeadline.feedView.setupWithItem(item)
      cell.content.actionHandler = handler(for: item, card: card, indexPath: indexPath)
      cell.content.rateCardActionHandler = { [weak self] action in
        self?.actionHandler(.rateCardAction(action))
      }
      cell.content.contextMenu = contextMenu(for: item, card: card, indexPath: indexPath)
      return cell
    case .group(let items, let title, let direction, _):
      let groupView: FeedGroupView
      let cell: UICollectionViewCell
      switch direction {
      case .horizontal:
        let horizontalCell =
          collectionView.dequeueReusableCell(for: indexPath)
          as FeedCardCell<HorizontalFeedGroupView>
        groupView = horizontalCell.content
        cell = horizontalCell
      case .vertical:
        let verticalCell =
          collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<VerticalFeedGroupView>
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
      let cell =
        collectionView.dequeueReusableCell(for: indexPath) as FeedCardCell<NumberedFeedGroupView>
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

  private func handler(
    from feedList: @escaping (Int) -> FeedItem,
    card: FeedCard,
    indexPath: IndexPath
  ) -> (Int, FeedItemAction) -> Void {
    return { [weak self] index, action in
      self?.actionHandler(
        .itemAction(action, context: .init(item: feedList(index), card: card, indexPath: indexPath))
      )
    }
  }

  private func handler(
    for item: FeedItem,
    card: FeedCard,
    indexPath: IndexPath
  ) -> (Int, FeedItemAction) -> Void {
    return handler(from: { _ in item }, card: card, indexPath: indexPath)
  }

  private func contextMenu(
    from feedList: @escaping (Int) -> FeedItem,
    card: FeedCard,
    indexPath: IndexPath
  ) -> FeedItemMenu {
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
        .init(
          title: Strings.openNewTabButtonTitle,
          image: UIImage(braveSystemNamed: "leo.browser.mobile-tab-new"),
          handler: mapDeferredHandler(openInNewTabHandler)
        )
      }

      var openInNewPrivateTab: UIAction {
        .init(
          title: Strings.openNewPrivateTabButtonTitle,
          image: UIImage(braveSystemNamed: "leo.product.private-window"),
          handler: mapDeferredHandler(openInNewPrivateTabHandler)
        )
      }

      var disableSource: UIAction {
        .init(
          title: String(format: Strings.BraveNews.disablePublisherContent, item.source.name),
          image: UIImage(braveSystemNamed: "leo.eye.off"),
          attributes: .destructive,
          handler: mapDeferredHandler(toggleSourceHandler)
        )
      }

      var enableSource: UIAction {
        .init(
          title: String(format: Strings.BraveNews.enablePublisherContent, item.source.name),
          image: UIImage(braveSystemNamed: "leo.eye.on"),
          handler: mapDeferredHandler(toggleSourceHandler)
        )
      }

      let manageActions = [
        self.dataSource.isSourceHidden(item.source) ? enableSource : disableSource
      ]

      var children: [UIMenu] = [
        UIMenu(title: "", options: [.displayInline], children: [openInNewTab, openInNewPrivateTab])
      ]
      if context.item.content.contentType != .sponsor {
        children.append(UIMenu(title: "", options: [.displayInline], children: manageActions))
      }
      return UIMenu(title: item.content.url?.absoluteString ?? "", children: children)
    }
  }

  private func contextMenu(for item: FeedItem, card: FeedCard, indexPath: IndexPath) -> FeedItemMenu
  {
    return contextMenu(from: { _ in item }, card: card, indexPath: indexPath)
  }

  // MARK: - P3A

  private func recordWeeklyAdsViewedP3A(adViewed: Bool) {
    var storage = P3ATimedStorage<Int>.adsViewedStorage
    if adViewed {
      storage.add(value: 1, to: Date())
    }
    UmaHistogramRecordValueToBucket(
      "Brave.Today.WeeklyDisplayAdsViewedCount",
      buckets: [
        0,
        1,
        .r(2...4),
        .r(5...12),
        .r(13...20),
        .r(21...40),
        .r(41...80),
        .r(81...),
      ],
      value: storage.combinedValue
    )
  }

  private func recordWeeklyCardsViewedP3A(cardViewed: Bool) {
    var storage = P3ATimedStorage<Int>.cardsViewCount
    if cardViewed {
      storage.add(value: 1, to: Date())
    }
    UmaHistogramRecordValueToBucket(
      "Brave.Today.WeeklyTotalCardViews",
      buckets: [
        0,
        1,
        .r(2...10),
        .r(11...20),
        .r(21...40),
        .r(41...80),
        .r(81...100),
        .r(101...),
      ],
      value: storage.combinedValue
    )
  }

  private func recordWeeklyMaxRowsViewedP3A() {
    var storage = P3ATimedStorage<Int>.rowsViewedCount
    storage.replaceTodaysRecordsIfLargest(value: viewedCards.count)
    UmaHistogramRecordValueToBucket(
      "Brave.Today.WeeklyMaxCardViewsCount",
      buckets: [
        0,  // won't ever be sent
        1,
        .r(2...4),
        .r(5...12),
        .r(13...20),
        .r(21...40),
        .r(41...80),
        .r(81...),
      ],
      value: storage.maximumDaysCombinedValue
    )
  }
}

extension FeedItemView {
  func setupWithItem(_ feedItem: FeedItem, isBrandVisible: Bool = true) {
    titleLabel.text = feedItem.content.title
    dateLabel.text = RelativeDateTimeFormatter().localizedString(
      for: feedItem.content.publishTime,
      relativeTo: Date()
    )
    if feedItem.content.imageURL == nil {
      thumbnailImageView.isHidden = true
    } else {
      thumbnailImageView.isHidden = false

      var url = URL(string: "data:image/svg+xml;charset=utf-8,%3Csvg xmlns='http://www.w3.org/2000/svg' width='18' height='18' fill='none' viewBox='0 0 18 18'%3E%3Cpath fill='%23000' d='M7.223 11.831c.489.36 1.032.646 1.614.845l.616-1.798a4.1 4.1 0 1 1 4.04-.803l1.256 1.426a6 6 0 0 0 1.41-7.161 6 6 0 1 0-8.936 7.491'/%3E%3Cpath fill='%23000' d='M10.646 6.235A6 6 0 0 0 9.05 5.36l-.65 1.786a4.1 4.1 0 1 1-4.053.729l-1.23-1.449a6 6 0 1 0 7.53-.192'/%3E%3C/svg%3E")
      if feedItem.content.imageURL!.absoluteString.contains(".pad") {
        url = feedItem.content.imageURL!
        print("🖼️ BRAVE NEWS: Loading image from URL: \(feedItem.content.imageURL!.absoluteString)")
      }
      thumbnailImageView.sd_setImage(
        with: url,
        placeholderImage: nil,
        options: .avoidAutoSetImage,
        completed: { (image, _, cacheType, _) in
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
        }
      )
    }

    brandContainerView.textLabel.text = nil
    if isBrandVisible {
      brandContainerView.textLabel.text = feedItem.source.name
    }
  }
}

extension P3ATimedStorage where Value == Int {
  fileprivate static var adsViewedStorage: Self { .init(name: "ads-viewed", lifetimeInDays: 7) }
  fileprivate static var cardsViewCount: Self {
    .init(name: "news-cards-view-count", lifetimeInDays: 7)
  }
  fileprivate static var rowsViewedCount: Self {
    .init(name: "news-cards-rows-count", lifetimeInDays: 7)
  }
}
