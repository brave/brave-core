// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveNews
import BraveShared
import BraveUI
import Combine
import CoreData
import Data
import DesignSystem
import Growth
import Preferences
import Shared
import SnapKit
import SwiftUI
import UIKit

/// The behavior for sizing sections when the user is in landscape orientation
enum NTPLandscapeSizingBehavior {
  /// The section is given half the available space
  ///
  /// Layout is decided by device type (iPad vs iPhone)
  case halfWidth
  /// The section is given the full available space
  ///
  /// Layout is up to the section to define
  case fullWidth
}

/// A section that will be shown in the NTP. Sections are responsible for the
/// layout and interaction of their own items
protocol NTPSectionProvider: NSObject, UICollectionViewDelegateFlowLayout,
  UICollectionViewDataSource
{
  /// Register cells and supplimentary views for your section to
  /// `collectionView`
  func registerCells(to collectionView: UICollectionView)
  /// The defined behavior when the user is in landscape.
  ///
  /// Defaults to `halfWidth`, which will only give half of the available
  /// width to the section (and adjust layout automatically based on device)
  var landscapeBehavior: NTPLandscapeSizingBehavior { get }
}

extension NTPSectionProvider {
  var landscapeBehavior: NTPLandscapeSizingBehavior { .halfWidth }
  /// The bounding size for auto-sizing cells, bound to the maximum available
  /// width in the collection view, taking into account safe area insets and
  /// insets for that given section
  func fittingSizeForCollectionView(_ collectionView: UICollectionView, section: Int) -> CGSize {
    let sectionInset: UIEdgeInsets
    if let flowLayout = collectionView.collectionViewLayout as? UICollectionViewFlowLayout {
      if let flowLayoutDelegate = collectionView.delegate as? UICollectionViewDelegateFlowLayout {
        sectionInset =
          flowLayoutDelegate.collectionView?(
            collectionView,
            layout: collectionView.collectionViewLayout,
            insetForSectionAt: section
          ) ?? flowLayout.sectionInset
      } else {
        sectionInset = flowLayout.sectionInset
      }
    } else {
      sectionInset = .zero
    }
    return CGSize(
      width: collectionView.bounds.width - collectionView.safeAreaInsets.left
        - collectionView.safeAreaInsets.right - sectionInset.left - sectionInset.right,
      height: 1000
    )
  }
}

/// A section provider that can be observed for changes to tell the
/// `NewTabPageViewController` to reload its section
protocol NTPObservableSectionProvider: NTPSectionProvider {
  var sectionDidChange: (() -> Void)? { get set }
}

protocol NewTabPageDelegate: AnyObject {
  func focusURLBar()
  func navigateToInput(_ input: String, inNewTab: Bool, switchingToPrivateMode: Bool)
  func handleFavoriteAction(favorite: Favorite, action: BookmarksAction)
  func brandedImageCalloutActioned(_ state: BrandedImageCalloutState)
  func tappedQRCodeButton(url: URL)
  func showNTPOnboarding()
}

/// The new tab page. Shows users a variety of information, including stats and
/// favourites
class NewTabPageViewController: UIViewController {
  weak var delegate: NewTabPageDelegate?

  var ntpStatsOnboardingFrame: CGRect? {
    guard let section = sections.firstIndex(where: { $0 is StatsSectionProvider }) else {
      return nil
    }

    if let cell = collectionView.cellForItem(at: IndexPath(item: 0, section: section))
      as? NewTabCenteredCollectionViewCell<BraveShieldStatsView>
    {
      return cell.contentView.convert(cell.contentView.frame, to: view)
    }
    return nil
  }

  /// The modules to show on the new tab page
  private var sections: [NTPSectionProvider] = []

  private let layout = NewTabPageFlowLayout()
  private let collectionView: NewTabCollectionView
  private weak var browserTab: Tab?
  private let rewards: BraveRewards

  private var background: NewTabPageBackground
  private let backgroundView = NewTabPageBackgroundView()
  private let backgroundButtonsView: NewTabPageBackgroundButtonsView
  private var videoAdPlayer: NewTabPageVideoAdPlayer?
  private var videoButtonsView = NewTabPageVideoAdButtonsView()

  /// A gradient to display over background images to ensure visibility of
  /// the NTP contents and sponsored logo
  ///
  /// Only should be displayed when the user has background images enabled
  let gradientView = GradientView(
    colors: [
      UIColor(white: 0.0, alpha: 0.5),
      UIColor(white: 0.0, alpha: 0.0),
      UIColor(white: 0.0, alpha: 0.3),
    ],
    positions: [0, 0.5, 0.8],
    startPoint: .zero,
    endPoint: CGPoint(x: 0, y: 1)
  )

  private let feedDataSource: FeedDataSource
  private let feedOverlayView = NewTabPageFeedOverlayView()
  private var preventReloadOnBraveNewsEnabledChange = false

  private let notifications: NewTabPageNotifications
  private var cancellables: Set<AnyCancellable> = []
  private let privateBrowsingManager: PrivateBrowsingManager

  private let p3aHelper: NewTabPageP3AHelper

  init(
    tab: Tab,
    profile: Profile,
    dataSource: NTPDataSource,
    feedDataSource: FeedDataSource,
    rewards: BraveRewards,
    privateBrowsingManager: PrivateBrowsingManager,
    p3aHelper: NewTabPageP3AHelper
  ) {
    self.browserTab = tab
    self.rewards = rewards
    self.feedDataSource = feedDataSource
    self.privateBrowsingManager = privateBrowsingManager
    self.backgroundButtonsView = NewTabPageBackgroundButtonsView(
      privateBrowsingManager: privateBrowsingManager
    )
    self.p3aHelper = p3aHelper
    background = NewTabPageBackground(dataSource: dataSource, rewards: rewards)
    notifications = NewTabPageNotifications(rewards: rewards)
    collectionView = NewTabCollectionView(frame: .zero, collectionViewLayout: layout)
    super.init(nibName: nil, bundle: nil)

    Preferences.NewTabPage.showNewTabPrivacyHub.observe(from: self)
    Preferences.NewTabPage.showNewTabFavourites.observe(from: self)

    sections = [
      StatsSectionProvider(
        isPrivateBrowsing: tab.isPrivate,
        openPrivacyHubPressed: { [weak self] in
          if self?.privateBrowsingManager.isPrivateBrowsing == true {
            return
          }

          let host = UIHostingController(
            rootView: PrivacyReportsManager.prepareView(
              isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing
            )
          )
          host.rootView.onDismiss = { [weak self] in
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
              guard let self = self else { return }

              // Handle App Rating
              // User finished viewing the privacy report (tapped close)
              AppReviewManager.shared.handleAppReview(for: .revised, using: self)
            }
          }

          host.rootView.openPrivacyReportsUrl = { [weak self] in
            self?.delegate?.navigateToInput(
              URL.brave.privacyFeatures.absoluteString,
              inNewTab: false,
              // Privacy Reports view is unavailable in private mode.
              switchingToPrivateMode: false
            )
          }

          self?.present(host, animated: true)
        },
        hidePrivacyHubPressed: { [weak self] in
          self?.hidePrivacyHub()
        }
      ),
      FavoritesSectionProvider(
        action: { [weak self] bookmark, action in
          self?.handleFavoriteAction(favorite: bookmark, action: action)
        },
        legacyLongPressAction: { [weak self] alertController in
          self?.present(alertController, animated: true)
        },
        isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing
      ),
      FavoritesOverflowSectionProvider(action: { [weak self] in
        self?.delegate?.focusURLBar()
      }),
    ]

    var isBackgroundNTPSI = false
    if let ntpBackground = background.currentBackground, case .sponsoredMedia = ntpBackground {
      isBackgroundNTPSI = true
    }
    let ntpDefaultBrowserCalloutProvider = NTPDefaultBrowserCalloutProvider(
      isBackgroundNTPSI: isBackgroundNTPSI
    )

    // This is a one-off view, adding it to the NTP only if necessary.
    if ntpDefaultBrowserCalloutProvider.shouldShowCallout() {
      sections.insert(ntpDefaultBrowserCalloutProvider, at: 0)
    }

    if !privateBrowsingManager.isPrivateBrowsing {
      sections.append(
        BraveNewsSectionProvider(
          dataSource: feedDataSource,
          rewards: rewards,
          actionHandler: { [weak self] in
            self?.handleBraveNewsAction($0)
          }
        )
      )
      layout.braveNewsSection = sections.firstIndex(where: { $0 is BraveNewsSectionProvider })
    }

    collectionView.do {
      $0.delegate = self
      $0.dataSource = self
      $0.dragDelegate = self
      $0.dropDelegate = self
    }

    background.changed = { [weak self] in
      guard let self else { return }
      setupBackgroundImage()

      let isTabVisible = viewIfLoaded?.window != nil
      setupBackgroundVideoIfNeeded(shouldCreatePlayer: isTabVisible)
      // Load the video asset here, as viewDidAppear is not called when the view
      // is already visible
      if isTabVisible {
        videoPlayer?.loadAndAutoplayVideoAssetIfNeeded(
          shouldAutoplay: false
        )
      }
    }

    Preferences.BraveNews.isEnabled.observe(from: self)
    feedDataSource.$state
      .scan((.initial, .initial), { ($0.1, $1) })
      .receive(on: DispatchQueue.main)
      .sink { [weak self] (oldState, newState) in
        self?.handleFeedStateChange(oldState, newState)
      }
      .store(in: &cancellables)
    NotificationCenter.default.addObserver(
      self,
      selector: #selector(checkForUpdatedFeed),
      name: UIApplication.didBecomeActiveNotification,
      object: nil
    )

    let braveNewsFeatureUsage = P3AFeatureUsage.braveNewsFeatureUsage
    if isBraveNewsVisible && Preferences.BraveNews.isEnabled.value {
      braveNewsFeatureUsage.recordHistogram()
      recordBraveNewsDaysUsedP3A()
    }

    recordNewTabCreatedP3A()
    recordBraveNewsWeeklyUsageCountP3A()
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    view.addSubview(backgroundView)
    view.insertSubview(gradientView, aboveSubview: backgroundView)
    view.addSubview(videoButtonsView)
    view.addSubview(collectionView)
    view.addSubview(feedOverlayView)

    collectionView.backgroundView = backgroundButtonsView

    feedOverlayView.headerView.settingsButton.addTarget(
      self,
      action: #selector(tappedBraveNewsSettings),
      for: .touchUpInside
    )
    if !AppConstants.isOfficialBuild {
      // Add a shortcut only available in local builds
      feedOverlayView.headerView.settingsButton.addGestureRecognizer(
        UILongPressGestureRecognizer(
          target: self,
          action: #selector(longPressedBraveNewsSettingsButton)
        )
      )
    }
    feedOverlayView.newContentAvailableButton.addTarget(
      self,
      action: #selector(tappedNewContentAvailable),
      for: .touchUpInside
    )

    backgroundButtonsView.tappedActiveButton = { [weak self] sender in
      self?.tappedActiveBackgroundButton(sender)
    }

    setupBackgroundImage()
    setupBackgroundVideoIfNeeded(shouldCreatePlayer: true)
    backgroundView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    videoButtonsView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    collectionView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    feedOverlayView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    gradientView.snp.makeConstraints {
      $0.edges.equalTo(backgroundView)
    }

    sections.enumerated().forEach { (index, provider) in
      provider.registerCells(to: collectionView)
      if let observableProvider = provider as? NTPObservableSectionProvider {
        observableProvider.sectionDidChange = { [weak self] in
          guard let self = self else { return }
          if self.parent != nil {
            UIView.performWithoutAnimation {
              // As of iOS 16.4, reloadSections seems to do some sort of validation of the underlying data
              // for other sections that aren't being refreshed. This can cause assertions for sections that
              // may need to reload in the same batch but don't. Since we don't animate this section anyways
              // we can just switch to `reloadData` here.
              self.collectionView.reloadData()
            }
          }
          self.collectionView.collectionViewLayout.invalidateLayout()
        }
      }
    }
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    checkForUpdatedFeed()
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()

    collectionView.reloadData()

    // Make sure that imageView has a frame calculated before we attempt
    // to use it.
    backgroundView.layoutIfNeeded()

    updateVideoAdPlayer()

    calculateBackgroundCenterPoints()
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)

    reportSponsoredBackgroundEvent(.servedImpression) { [weak self] _ in
      self?.reportSponsoredBackgroundEvent(.viewedImpression)
    }

    videoAdPlayer?.loadAndAutoplayVideoAssetIfNeeded(
      shouldAutoplay: shouldShowBackgroundVideo()
    )

    presentNotification()

    DispatchQueue.main.asyncAfter(deadline: .now() + 0.50) {
      self.delegate?.showNTPOnboarding()
    }
  }

  override func viewSafeAreaInsetsDidChange() {
    super.viewSafeAreaInsetsDidChange()

    backgroundButtonsView.collectionViewSafeAreaInsets = view.safeAreaInsets
  }

  override func willMove(toParent parent: UIViewController?) {
    super.willMove(toParent: parent)

    backgroundView.imageView.image = parent == nil ? nil : background.backgroundImage

    if parent == nil {
      videoAdPlayer?.cancelPlayIfNeeded()
      videoAdPlayer?.resetPlayer()
    } else {
      videoAdPlayer?.createPlayer()
      videoAdPlayer?.seekToStopFrame()
    }
    backgroundView.playerLayer.player = videoAdPlayer?.player
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    if previousTraitCollection?.verticalSizeClass
      != traitCollection.verticalSizeClass
    {
      calculateBackgroundCenterPoints()
    }
  }

  // MARK: - Background

  /// Hide any visible sponsored image notification if the current background
  /// is no longer a sponsored image. If the visible notification is not
  /// for sponsored images, this does nothing.
  private func hideVisibleSponsoredImageNotification() {
    if case .brandedImages = visibleNotification {
      guard let background = background.currentBackground else {
        hideNotification()
        return
      }
      switch background {
      case .image, .superReferral:
        hideNotification()
      case .sponsoredMedia:
        // Current background is still a sponsored image so it can stay
        // visible
        break
      }
    }
  }

  private func fadeOutCollectionViewAndShowVideoButtons() {
    self.videoButtonsView.isHidden = false
    self.gradientView.isHidden = true

    UIView.animate(
      withDuration: 0.3,
      animations: { [weak self] in
        self?.collectionView.alpha = 0
      },
      completion: { [weak self] _ in
        self?.collectionView.isHidden = true
        self?.collectionView.alpha = 1
      }
    )
  }

  private func fadeInCollectionViewAndHideVideoButtons() {
    videoButtonsView.isHidden = true
    gradientView.isHidden = false
    collectionView.isHidden = false
    collectionView.alpha = 0
    UIView.animate(
      withDuration: 0.3,
      animations: { [weak self] in
        self?.collectionView.alpha = 1
        self?.videoAdPlayer?.seekToStopFrame()
      }
    )
  }

  func setupBackgroundVideoIfNeeded(shouldCreatePlayer: Bool) {
    videoButtonsView.isHidden = true

    guard let backgroundVideoPath = background.backgroundVideoPath else {
      videoAdPlayer = nil
      backgroundView.resetPlayerLayer()
      backgroundButtonsView.resetVideoBackgroundButtons()
      return
    }

    gradientView.isHidden = false
    videoAdPlayer = NewTabPageVideoAdPlayer(backgroundVideoPath)
    if shouldCreatePlayer {
      videoAdPlayer?.createPlayer()
    }

    backgroundView.setupPlayerLayer(backgroundVideoPath, player: videoAdPlayer?.player)

    videoButtonsView.tappedBackgroundVideo = { [weak videoAdPlayer] in
      guard let videoAdPlayer else {
        return false
      }
      return videoAdPlayer.togglePlay()
    }
    videoButtonsView.tappedCancelButton = { [weak videoAdPlayer] in
      videoAdPlayer?.cancelPlayIfNeeded()
    }

    backgroundButtonsView.activeButton = .none
    backgroundButtonsView.tappedPlayButton = { [weak self] in
      self?.videoAdPlayer?.startPlayback()
    }
    backgroundButtonsView.tappedBackgroundDuringAutoplay = { [weak self] in
      self?.videoAdPlayer?.startPlayback()
    }

    videoAdPlayer?.didCancelPlaybackEvent = { [weak self] in
      guard let self = self else { return }
      self.fadeInCollectionViewAndHideVideoButtons()
    }
    videoAdPlayer?.didStartAutoplayEvent = { [weak self] in
      self?.backgroundButtonsView.videoAutoplayStarted()
    }
    videoAdPlayer?.didFinishAutoplayEvent = { [weak self] in
      guard let self = self else { return }
      self.backgroundButtonsView.videoAutoplayFinished()
      if case .sponsoredMedia(let background) = self.background.currentBackground {
        self.backgroundButtonsView.activeButton = .brandLogo(background.logo)
      }
      self.backgroundButtonsView.alpha = 0
      UIView.animate(
        withDuration: 0.3,
        animations: { [weak self] in
          self?.backgroundButtonsView.alpha = 1
        }
      )
    }
    videoAdPlayer?.didFinishPlaybackEvent = { [weak self] in
      guard let self = self else { return }
      self.fadeInCollectionViewAndHideVideoButtons()
      self.reportSponsoredBackgroundEvent(.media100)
    }
    videoAdPlayer?.didStartPlaybackEvent = { [weak self] in
      guard let self = self else { return }
      self.fadeOutCollectionViewAndShowVideoButtons()
      self.reportSponsoredBackgroundEvent(.mediaPlay)
    }
    videoAdPlayer?.didPlay25PercentEvent = { [weak self] in
      self?.reportSponsoredBackgroundEvent(.media25)
    }
  }

  private func shouldShowBackgroundVideo() -> Bool {
    let isLandscape = view.window?.windowScene?.interfaceOrientation.isLandscape == true
    return !(isLandscape && UIDevice.isPhone)
  }

  private func updateVideoAdPlayer() {
    backgroundView.playerLayer.frame = view.bounds

    if shouldShowBackgroundVideo() {
      backgroundView.playerLayer.isHidden = false
    } else {
      // Hide the player layer in landscape mode on iPhone.
      backgroundView.playerLayer.isHidden = true
      videoAdPlayer?.cancelPlayIfNeeded()
    }
  }

  func setupBackgroundImage() {
    collectionView.reloadData()

    hideVisibleSponsoredImageNotification()

    if let background = background.currentBackground {
      switch background {
      case .image(let background):
        if case let name = background.author, !name.isEmpty {
          backgroundButtonsView.activeButton = .imageCredit(name)
        } else {
          backgroundButtonsView.activeButton = .none
        }
      case .sponsoredMedia(let background):
        backgroundButtonsView.activeButton = .brandLogo(background.logo)
      case .superReferral:
        backgroundButtonsView.activeButton = .qrCode
      }
    } else {
      backgroundButtonsView.activeButton = .none
    }

    gradientView.isHidden = background.backgroundImage == nil
    backgroundView.imageView.image = background.backgroundImage
  }

  private func calculateBackgroundCenterPoints() {

    // Only iPhone portrait looking devices have their center of the image offset adjusted.
    // In other cases the image is always centered.
    guard let image = backgroundView.imageView.image,
      traitCollection.horizontalSizeClass == .compact
        && traitCollection.verticalSizeClass == .regular
    else {
      // Reset the previously calculated offset.
      backgroundView.updateImageXOffset(by: 0)
      return
    }

    // If no focal point provided we do nothing. The image is centered by default.
    guard let focalPoint = background.currentBackground?.focalPoint else {
      return
    }

    let focalX = focalPoint.x

    // Calculate the sizing difference between `image` and `imageView` to determine the pixel difference ratio.
    // Most image calculations have to use this property to get coordinates right.
    let sizeRatio = backgroundView.imageView.frame.size.height / image.size.height

    // How much the image should be offset according to the set focal point coordinate.
    // We calculate it by looking how much to move the image away from the center of the image.
    let focalXOffset = ((image.size.width / 2) - focalX) * sizeRatio

    // Amount of image space which is cropped on one side, not visible on the screen.
    // We use this info to prevent going of out image bounds when updating the `x` offset.
    let extraHorizontalSpaceOnOneSide =
      ((image.size.width * sizeRatio) - backgroundView.frame.width) / 2

    // The offset proposed by the focal point might be too far away from image's center
    // resulting in not having anough image space to cover entire width of the view and leaving blank space.
    // If the focal offset goes out of bounds we center it to the maximum amount we can where the entire
    // image is able to cover the view.
    let realisticXOffset =
      abs(focalXOffset) > extraHorizontalSpaceOnOneSide
      ? extraHorizontalSpaceOnOneSide : focalXOffset

    backgroundView.updateImageXOffset(by: realisticXOffset)
  }

  private func reportSponsoredBackgroundEvent(
    _ event: BraveAds.NewTabPageAdEventType,
    completion: ((_ success: Bool) -> Void)? = nil
  ) {
    if let tab = browserTab,
      case .sponsoredMedia(let sponsoredBackground) = background.currentBackground
    {
      let eventType: NewTabPageP3AHelper.EventType? = {
        switch event {
        case .clicked: return .tapped
        case .viewedImpression: return .viewed
        case .mediaPlay: return .mediaPlay
        case .media25: return .media25
        case .media100: return .media100
        default: return nil
        }
      }()
      if let eventType {
        p3aHelper.recordEvent(eventType, on: tab, for: sponsoredBackground)
      }
      rewards.ads.triggerNewTabPageAdEvent(
        background.wallpaperId.uuidString,
        creativeInstanceId: sponsoredBackground.creativeInstanceId,
        eventType: event,
        completion: { success in
          completion?(success)
        }
      )
    }
  }

  // MARK: - Notifications

  private var notificationController: UIViewController?
  private var visibleNotification: NewTabPageNotifications.NotificationType?
  private var notificationShowing: Bool {
    notificationController?.parent != nil
  }

  private func presentNotification() {
    if privateBrowsingManager.isPrivateBrowsing || notificationShowing {
      return
    }

    var isShowingSponseredImage = false
    if case .sponsoredMedia = background.currentBackground {
      isShowingSponseredImage = true
    }

    guard
      let notification = notifications.notificationToShow(
        isShowingBackgroundImage: background.currentBackground != nil,
        isShowingSponseredImage: isShowingSponseredImage
      )
    else {
      return
    }

    var vc: UIViewController?

    switch notification {
    case .brandedImages(let state):
      if Preferences.NewTabPage.atleastOneNTPNotificationWasShowed.value { return }

      guard let notificationVC = NTPNotificationViewController(state: state, rewards: rewards)
      else { return }

      notificationVC.closeHandler = { [weak self] in
        self?.notificationController = nil
      }

      notificationVC.learnMoreHandler = { [weak self] in
        self?.delegate?.brandedImageCalloutActioned(state)
      }

      vc = notificationVC
    }

    guard let viewController = vc else { return }
    notificationController = viewController
    visibleNotification = notification

    DispatchQueue.main.asyncAfter(deadline: .now() + 1.5) { [weak self] in
      guard let self = self else { return }

      if case .brandedImages = notification {
        Preferences.NewTabPage.atleastOneNTPNotificationWasShowed.value = true
      }

      self.addChild(viewController)
      self.view.addSubview(viewController.view)
    }
  }

  private func hideNotification() {
    guard let controller = notificationController else { return }
    controller.willMove(toParent: nil)
    controller.removeFromParent()
    controller.view.removeFromSuperview()
    notificationController = nil
  }

  // MARK: - Brave News

  private var newsArticlesOpened: Set<FeedItem.ID> = []

  private func handleBraveNewsAction(_ action: BraveNewsSectionProvider.Action) {
    switch action {
    case .optInCardAction(.closedButtonTapped):
      Preferences.BraveNews.isShowingOptIn.value = false
      if let section = layout.braveNewsSection,
        collectionView.numberOfItems(inSection: section) != 0
      {
        collectionView.deleteItems(at: [IndexPath(item: 0, section: section)])
      }

      // We check if first item exists before scrolling up to it.
      // This should never happen since first item is our shields stats view.
      // However we saw it crashing in XCode logs, see #4202.
      let firstItemIndexPath = IndexPath(item: 0, section: 0)
      if let itemCount = collectionView.dataSource?.collectionView(
        collectionView,
        numberOfItemsInSection: 0
      ),
        itemCount > 0,  // Only scroll if the section has items, otherwise it will crash.
        collectionView.dataSource?
          .collectionView(collectionView, cellForItemAt: firstItemIndexPath) != nil
      {
        collectionView.scrollToItem(at: firstItemIndexPath, at: .top, animated: true)
      } else {
        // Cannot scorll to deleted item index.
        // Collection-View datasource never changes or updates
        // Therefore we need to scroll to offset 0.
        // See: #4575.
        collectionView.setContentOffset(.zero, animated: true)
      }
      collectionView.verticalScrollIndicatorInsets = .zero
      UIView.animate(withDuration: 0.25) {
        self.feedOverlayView.headerView.alpha = 0.0
        self.backgroundButtonsView.alpha = 1.0
      }
    case .optInCardAction(.learnMoreButtonTapped):
      delegate?.navigateToInput(
        URL.brave.braveNewsPrivacy.absoluteString,
        inNewTab: false,
        switchingToPrivateMode: false
      )
    case .optInCardAction(.turnOnBraveNewsButtonTapped):
      preventReloadOnBraveNewsEnabledChange = true
      Preferences.BraveNews.userOptedIn.value = true
      Preferences.BraveNews.isEnabled.value = true
      rewards.ads.initialize { [weak self] _ in
        // Initialize ads if it hasn't already been done
        self?.loadFeedContents()
      }
    case .emptyCardTappedSourcesAndSettings:
      tappedBraveNewsSettings()
    case .errorCardTappedRefresh:
      loadFeedContents()
    case .moreBraveOffersTapped:
      delegate?.navigateToInput(
        URL.brave.braveOffers.absoluteString,
        inNewTab: false,
        switchingToPrivateMode: false
      )
    case .bravePartnerLearnMoreTapped:
      delegate?.navigateToInput(
        URL.brave.braveNews.absoluteString,
        inNewTab: false,
        switchingToPrivateMode: false
      )
    case .itemAction(.opened(let inNewTab, let switchingToPrivateMode), let context):
      guard let url = context.item.content.url else { return }
      let item = context.item
      if !switchingToPrivateMode, item.content.contentType == .partner,
        let creativeInstanceID = item.content.creativeInstanceID
      {
        rewards.ads.triggerPromotedContentAdEvent(
          item.content.urlHash,
          creativeInstanceId: creativeInstanceID,
          eventType: .clicked,
          completion: { _ in }
        )
      }
      if switchingToPrivateMode, Preferences.Privacy.privateBrowsingLock.value {
        self.askForLocalAuthentication { [weak self] success, error in
          if success {
            self?.delegate?.navigateToInput(
              url.absoluteString,
              inNewTab: inNewTab,
              switchingToPrivateMode: switchingToPrivateMode
            )
          }
        }
      } else {
        delegate?.navigateToInput(
          url.absoluteString,
          inNewTab: inNewTab,
          switchingToPrivateMode: switchingToPrivateMode
        )
      }
      // Donate Open Brave News Activity for Custom Suggestions
      let openBraveNewsActivity = ActivityShortcutManager.shared.createShortcutActivity(
        type: .openBraveNews
      )
      self.userActivity = openBraveNewsActivity
      openBraveNewsActivity.becomeCurrent()
      // Record P3A
      newsArticlesOpened.insert(item.id)
      recordBraveNewsArticlesVisitedP3A()
    case .itemAction(.toggledSource, let context):
      let isHidden = feedDataSource.isSourceHidden(context.item.source)
      feedDataSource.toggleSourceHidden(context.item.source, hidden: !isHidden)
      if !isHidden {
        let alert = FeedActionAlertView.feedDisabledAlertView(for: context.item)
        alert.present(on: self)
      }
    case .inlineContentAdAction(.opened(let inNewTab, let switchingToPrivateMode), let ad):
      guard let url = ad.targetURL.asURL else { return }
      if !switchingToPrivateMode {
        rewards.ads.triggerInlineContentAdEvent(
          ad.placementID,
          creativeInstanceId: ad.creativeInstanceID,
          eventType: .clicked,
          completion: { _ in }
        )
      }
      delegate?.navigateToInput(
        url.absoluteString,
        inNewTab: inNewTab,
        switchingToPrivateMode: switchingToPrivateMode
      )
    case .inlineContentAdAction(.toggledSource, _):
      // Inline content ads have no source
      break
    case .rateCardAction(.rateBrave):
      Preferences.Review.newsCardShownDate.value = Date()
      guard
        let writeReviewURL = URL(
          string: "https://itunes.apple.com/app/id1052879175?action=write-review"
        )
      else {
        return
      }
      UIApplication.shared.open(writeReviewURL)
      feedDataSource.setNeedsReloadCards()
      loadFeedContents()
    case .rateCardAction(.hideCard):
      Preferences.Review.newsCardShownDate.value = Date()
      feedDataSource.setNeedsReloadCards()
      loadFeedContents()
    }
  }

  private var newContentAvailableDismissTimer: Timer? {
    didSet {
      oldValue?.invalidate()
    }
  }

  private func handleFeedStateChange(
    _ oldValue: FeedDataSource.State,
    _ newValue: FeedDataSource.State
  ) {
    guard let section = layout.braveNewsSection, parent != nil else { return }

    func _completeLoading() {
      if Preferences.BraveNews.isShowingOptIn.value {
        Preferences.BraveNews.isShowingOptIn.value = false
      }
      UIView.animate(
        withDuration: 0.2,
        animations: {
          self.feedOverlayView.loaderView.alpha = 0.0
        },
        completion: { _ in
          self.feedOverlayView.loaderView.stop()
          self.feedOverlayView.loaderView.alpha = 1.0
          self.feedOverlayView.loaderView.isHidden = true
        }
      )
      if collectionView.contentOffset.y == collectionView.contentInset.top {
        collectionView.reloadData()
        collectionView.layoutIfNeeded()
        let cells = collectionView.indexPathsForVisibleItems
          .filter { $0.section == section }
          .compactMap(collectionView.cellForItem(at:))
        cells.forEach { cell in
          cell.transform = .init(translationX: 0, y: 200)
          UIView.animate(
            withDuration: 0.5,
            delay: 0,
            usingSpringWithDamping: 1.0,
            initialSpringVelocity: 0,
            options: [.beginFromCurrentState],
            animations: {
              cell.transform = .identity
            },
            completion: nil
          )
        }
      } else {
        collectionView.reloadSections(IndexSet(integer: section))
      }
    }

    switch (oldValue, newValue) {
    case (.loading, .loading):
      // Nothing to do
      break
    case (
      .failure(let error1 as NSError),
      .failure(let error2 as NSError)
    ) where error1 == error2:
      // Nothing to do
      break
    case (
      .loading(.failure(let error1 as NSError)),
      .failure(let error2 as NSError)
    ) where error1 == error2:
      if let cell = collectionView.cellForItem(at: IndexPath(item: 0, section: section))
        as? FeedCardCell<BraveNewsErrorView>
      {
        cell.content.refreshButton.isLoading = false
      } else {
        _completeLoading()
      }
    case (_, .loading):
      if collectionView.contentOffset.y == collectionView.contentInset.top
        || collectionView.numberOfItems(inSection: section) == 0
      {
        feedOverlayView.loaderView.isHidden = false
        feedOverlayView.loaderView.start()

        let numberOfItems = collectionView.numberOfItems(inSection: section)
        if numberOfItems > 0 {
          collectionView.reloadSections(IndexSet(integer: section))
        }
      }
    case (.loading, _):
      _completeLoading()
    default:
      collectionView.reloadSections(IndexSet(integer: section))
    }
  }

  @objc private func checkForUpdatedFeed() {
    if !isBraveNewsVisible || Preferences.BraveNews.isShowingOptIn.value { return }
    if collectionView.contentOffset.y == collectionView.contentInset.top {
      // Reload contents if the user is not currently scrolled into the feed
      loadFeedContents()
    } else {
      if case .failure = feedDataSource.state {
        // Refresh button already exists on the users failure card
        return
      }
      // Possibly show the "new content available" button
      if feedDataSource.shouldLoadContent {
        feedOverlayView.showNewContentAvailableButton()
      }
    }
  }

  private func loadFeedContents(completion: (() -> Void)? = nil) {
    if !feedDataSource.shouldLoadContent {
      return
    }
    feedDataSource.load(completion)
  }

  private func hidePrivacyHub() {
    if Preferences.NewTabPage.hidePrivacyHubAlertShown.value {
      Preferences.NewTabPage.showNewTabPrivacyHub.value = false
      collectionView.reloadData()
    } else {
      let alert = UIAlertController(
        title: Strings.PrivacyHub.hidePrivacyHubWidgetActionTitle,
        message: Strings.PrivacyHub.hidePrivacyHubWidgetAlertDescription,
        preferredStyle: .alert
      )

      alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel))
      alert.addAction(
        UIAlertAction(
          title: Strings.PrivacyHub.hidePrivacyHubWidgetActionButtonTitle,
          style: .default
        ) { [weak self] _ in
          Preferences.NewTabPage.showNewTabPrivacyHub.value = false
          Preferences.NewTabPage.hidePrivacyHubAlertShown.value = true
          self?.collectionView.reloadData()
        }
      )

      UIImpactFeedbackGenerator(style: .medium).vibrate()
      present(alert, animated: true, completion: nil)
    }
  }

  // MARK: - Actions

  @objc private func tappedNewContentAvailable() {
    if case .loading = feedDataSource.state {
      return
    }
    let todayStart =
      collectionView.frame.height - feedOverlayView.headerView.bounds.height - 32 - 16
    newContentAvailableDismissTimer = nil
    feedOverlayView.newContentAvailableButton.isLoading = true
    loadFeedContents { [weak self] in
      guard let self = self else { return }
      self.feedOverlayView.hideNewContentAvailableButton()
      DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
        self.collectionView.setContentOffset(CGPoint(x: 0, y: todayStart), animated: true)
      }
    }
  }

  @objc private func tappedBraveNewsSettings() {
    let controller = NewsSettingsViewController(
      dataSource: feedDataSource,
      openURL: { [weak self] url in
        guard let self else { return }
        self.dismiss(animated: true)
        self.delegate?.navigateToInput(
          url.absoluteString,
          inNewTab: false,
          switchingToPrivateMode: false
        )
      }
    )
    controller.viewDidDisappear = { [weak self] in
      if Preferences.Review.braveNewsCriteriaPassed.value {
        AppReviewManager.shared.isRevisedReviewRequired = true
        Preferences.Review.braveNewsCriteriaPassed.value = false
      }
      self?.checkForUpdatedFeed()
    }
    let container = UINavigationController(rootViewController: controller)
    present(container, animated: true)
  }

  private func tappedActiveBackgroundButton(_ sender: UIControl) {
    guard let background = background.currentBackground else { return }
    switch background {
    case .image:
      presentImageCredit(sender)
    case .sponsoredMedia(let background):
      tappedSponsorButton(background.logo)
    case .superReferral(_, let code):
      tappedQRCode(code)
    }
  }

  private func tappedSponsorButton(_ logo: NTPSponsoredImageLogo) {
    UIImpactFeedbackGenerator(style: .medium).vibrate()
    if let url = logo.destinationURL {
      delegate?.navigateToInput(url.absoluteString, inNewTab: false, switchingToPrivateMode: false)
    }

    reportSponsoredBackgroundEvent(.clicked)
  }

  private func tappedQRCode(_ code: String) {
    // Super referrer websites come in format https://brave.com/r/REF_CODE
    let refUrl = URL(string: "https://brave.com/")?
      .appendingPathComponent("r")
      .appendingPathComponent(code)

    guard let url = refUrl else { return }
    delegate?.tappedQRCodeButton(url: url)
  }

  private func handleFavoriteAction(favorite: Favorite, action: BookmarksAction) {
    delegate?.handleFavoriteAction(favorite: favorite, action: action)
  }

  private func presentImageCredit(_ button: UIControl) {
    guard case .image(let background) = background.currentBackground else { return }

    let alert = UIAlertController(
      title: background.author,
      message: nil,
      preferredStyle: .actionSheet
    )

    if let creditURL = background.link {
      let websiteTitle = String(format: Strings.viewOn, creditURL.hostSLD.capitalizeFirstLetter)
      alert.addAction(
        UIAlertAction(title: websiteTitle, style: .default) { [weak self] _ in
          self?.delegate?.navigateToInput(
            creditURL.absoluteString,
            inNewTab: false,
            switchingToPrivateMode: false
          )
        }
      )
    }

    alert.popoverPresentationController?.sourceView = button
    alert.popoverPresentationController?.permittedArrowDirections = [.down, .up]
    alert.addAction(UIAlertAction(title: Strings.close, style: .cancel, handler: nil))

    UIImpactFeedbackGenerator(style: .medium).vibrate()
    present(alert, animated: true, completion: nil)
  }

  @objc private func longPressedBraveNewsSettingsButton() {
    assert(
      !AppConstants.isOfficialBuild,
      "Debug settings are not accessible on public builds"
    )
    let settings = BraveNewsDebugSettingsView(dataSource: feedDataSource) { [weak self] in
      self?.dismiss(animated: true)
    }
    let container = UINavigationController(
      rootViewController: UIHostingController(rootView: settings)
    )
    present(container, animated: true)
  }
}

extension NewTabPageViewController: PreferencesObserver {
  func preferencesDidChange(for key: String) {
    if key == Preferences.NewTabPage.showNewTabPrivacyHub.key
      || key == Preferences.NewTabPage.showNewTabFavourites.key
    {
      collectionView.reloadData()
      return
    }

    if !preventReloadOnBraveNewsEnabledChange {
      collectionView.reloadData()
    }
    if !isBraveNewsVisible {
      collectionView.verticalScrollIndicatorInsets = .zero
      feedOverlayView.headerView.alpha = 0.0
      backgroundButtonsView.alpha = 1.0
    }
    preventReloadOnBraveNewsEnabledChange = false
  }
}

// MARK: - UIScrollViewDelegate
extension NewTabPageViewController {
  var isBraveNewsVisible: Bool {
    return !privateBrowsingManager.isPrivateBrowsing
      && (Preferences.BraveNews.isEnabled.value || Preferences.BraveNews.isShowingOptIn.value)
  }

  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    for section in sections {
      section.scrollViewDidScroll?(scrollView)
    }
    guard isBraveNewsVisible, let newsSection = layout.braveNewsSection else { return }
    if collectionView.numberOfItems(inSection: newsSection) > 0 {
      // Hide the buttons as Brave News feeds appear
      backgroundButtonsView.alpha =
        1.0 - max(0.0, min(1.0, (scrollView.contentOffset.y - scrollView.contentInset.top) / 16))
      // Show the header as Brave News feeds appear
      // Offset of where Brave News starts
      let todayStart =
        collectionView.frame.height - feedOverlayView.headerView.bounds.height - 32 - 16
      // Offset of where the header should begin becoming visible
      let alphaInStart = collectionView.frame.height / 2.0
      let value = scrollView.contentOffset.y
      let alpha = max(0.0, min(1.0, (value - alphaInStart) / (todayStart - alphaInStart)))
      feedOverlayView.headerView.alpha = alpha

      if feedOverlayView.newContentAvailableButton.alpha != 0
        && !feedOverlayView.newContentAvailableButton.isLoading
      {
        let velocity = scrollView.panGestureRecognizer.velocity(in: scrollView).y
        if velocity > 0 && collectionView.contentOffset.y < todayStart {
          // Scrolling up
          self.feedOverlayView.hideNewContentAvailableButton()
        } else if velocity < 0 {
          // Scrolling down
          if newContentAvailableDismissTimer == nil {
            let timer = Timer(
              timeInterval: 4,
              repeats: false
            ) { [weak self] _ in
              guard let self = self else { return }
              self.feedOverlayView.hideNewContentAvailableButton()
              self.newContentAvailableDismissTimer = nil
            }
            // Adding the timer manually under `common` mode allows it to execute while the user
            // is scrolling through the feed rather than have to wait until input stops
            RunLoop.main.add(timer, forMode: .common)
            newContentAvailableDismissTimer = timer
          }
        }
      }

      if scrollView.contentOffset.y >= todayStart {
        recordBraveNewsUsageP3A()
      }
    }
  }

  /// Moves New Tab Page Scroll to start of Brave News - Used for shortcut
  func scrollToBraveNews() {
    // Offset of where Brave News starts
    let todayStart =
      collectionView.frame.height - feedOverlayView.headerView.bounds.height - 32 - 16
    collectionView.contentOffset.y = todayStart
  }

  // MARK: - P3A

  private func recordBraveNewsUsageP3A() {
    let braveNewsFeatureUsage = P3AFeatureUsage.braveNewsFeatureUsage
    if !isBraveNewsVisible || !Preferences.BraveNews.isEnabled.value
      || Calendar.current.startOfDay(for: Date()) == braveNewsFeatureUsage.lastUsageOption.value
    {
      // Don't have Brave News enabled, or already recorded todays usage, no need to do it again
      return
    }

    // Usage
    braveNewsFeatureUsage.recordUsage()
    var braveNewsWeeklyCount = P3ATimedStorage<Int>.braveNewsWeeklyCount
    braveNewsWeeklyCount.add(value: 1, to: Date())

    // Usage over the past month
    var braveNewsDaysUsedStorage = P3ATimedStorage<Int>.braveNewsDaysUsedStorage
    braveNewsDaysUsedStorage.replaceTodaysRecordsIfLargest(value: 1)
    recordBraveNewsDaysUsedP3A()

    // Weekly usage
    recordBraveNewsWeeklyUsageCountP3A()

    // General Usage
    UmaHistogramBoolean("Brave.Today.UsageDaily", true)
    UmaHistogramBoolean("Brave.Today.UsageMonthly", true)
  }

  private func recordBraveNewsWeeklyUsageCountP3A() {
    let storage = P3ATimedStorage<Int>.braveNewsWeeklyCount
    UmaHistogramRecordValueToBucket(
      "Brave.Today.WeeklySessionCount",
      buckets: [
        0,
        1,
        .r(2...3),
        .r(4...7),
        .r(8...12),
        .r(13...18),
        .r(19...25),
        .r(26...),
      ],
      value: storage.combinedValue
    )
  }

  private func recordBraveNewsDaysUsedP3A() {
    let storage = P3ATimedStorage<Int>.braveNewsDaysUsedStorage
    UmaHistogramRecordValueToBucket(
      "Brave.Today.DaysInMonthUsedCount",
      buckets: [
        0,
        1,
        2,
        .r(3...5),
        .r(6...10),
        .r(11...15),
        .r(16...20),
        .r(21...),
      ],
      value: storage.combinedValue
    )
  }

  private func recordBraveNewsArticlesVisitedP3A() {
    // Count is per NTP session, sends max value of the week
    var storage = P3ATimedStorage<Int>.braveNewsVisitedArticlesCount
    storage.replaceTodaysRecordsIfLargest(value: newsArticlesOpened.count)
    UmaHistogramRecordValueToBucket(
      "Brave.Today.WeeklyMaxCardVisitsCount",
      buckets: [
        0,  // won't ever be sent
        1,
        .r(2...3),
        .r(4...6),
        .r(7...10),
        .r(11...15),
        .r(16...),
      ],
      value: storage.maximumDaysCombinedValue
    )
  }

  private func recordNewTabCreatedP3A() {
    var newTabsStorage = P3ATimedStorage<Int>.newTabsCreatedStorage
    var sponsoredStorage = P3ATimedStorage<Int>.sponsoredNewTabsCreatedStorage

    newTabsStorage.add(value: 1, to: Date())
    let newTabsCreatedAnswer = newTabsStorage.maximumDaysCombinedValue

    if case .sponsoredMedia = background.currentBackground {
      sponsoredStorage.add(value: 1, to: Date())
    }

    UmaHistogramRecordValueToBucket(
      "Brave.NTP.NewTabsCreated.3",
      buckets: [
        0,
        1,
        2,
        3,
        4,
        .r(5...8),
        .r(9...15),
        .r(16...),
      ],
      value: newTabsCreatedAnswer
    )

    if newTabsCreatedAnswer > 0 {
      let sponsoredPercent = Int(
        (Double(sponsoredStorage.maximumDaysCombinedValue) / Double(newTabsCreatedAnswer)) * 100.0
      )
      UmaHistogramRecordValueToBucket(
        "Brave.NTP.SponsoredNewTabsCreated",
        buckets: [
          0,
          .r(0..<10),
          .r(10..<20),
          .r(20..<30),
          .r(30..<40),
          .r(40..<50),
          .r(50...),
        ],
        value: sponsoredPercent
      )
    }
  }
}

// MARK: - UICollectionViewDelegateFlowLayout
extension NewTabPageViewController: UICollectionViewDelegateFlowLayout {
  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    sections[indexPath.section].collectionView?(collectionView, didSelectItemAt: indexPath)
  }
  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    sizeForItemAt indexPath: IndexPath
  ) -> CGSize {
    sections[indexPath.section].collectionView?(
      collectionView,
      layout: collectionViewLayout,
      sizeForItemAt: indexPath
    ) ?? .zero
  }
  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    insetForSectionAt section: Int
  ) -> UIEdgeInsets {
    let sectionProvider = sections[section]
    var inset =
      sectionProvider.collectionView?(
        collectionView,
        layout: collectionViewLayout,
        insetForSectionAt: section
      ) ?? .zero
    if sectionProvider.landscapeBehavior == .halfWidth {
      let isIphone = UIDevice.isPhone
      let isLandscape = view.frame.width > view.frame.height
      if isLandscape {
        let availableWidth =
          collectionView.bounds.width - collectionView.safeAreaInsets.left
          - collectionView.safeAreaInsets.right
        if isIphone {
          inset.left = availableWidth / 2.0
        } else {
          inset.right = availableWidth / 2.0
        }
      }
    }
    return inset
  }
  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    minimumLineSpacingForSectionAt section: Int
  ) -> CGFloat {
    sections[section].collectionView?(
      collectionView,
      layout: collectionViewLayout,
      minimumLineSpacingForSectionAt: section
    ) ?? 0
  }
  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    minimumInteritemSpacingForSectionAt section: Int
  ) -> CGFloat {
    sections[section].collectionView?(
      collectionView,
      layout: collectionViewLayout,
      minimumInteritemSpacingForSectionAt: section
    ) ?? 0
  }
  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    referenceSizeForHeaderInSection section: Int
  ) -> CGSize {
    sections[section].collectionView?(
      collectionView,
      layout: collectionViewLayout,
      referenceSizeForHeaderInSection: section
    ) ?? .zero
  }
}

// MARK: - UICollectionViewDelegate
extension NewTabPageViewController: UICollectionViewDelegate {
  func collectionView(
    _ collectionView: UICollectionView,
    willDisplay cell: UICollectionViewCell,
    forItemAt indexPath: IndexPath
  ) {
    sections[indexPath.section].collectionView?(
      collectionView,
      willDisplay: cell,
      forItemAt: indexPath
    )
  }
  func collectionView(
    _ collectionView: UICollectionView,
    didEndDisplaying cell: UICollectionViewCell,
    forItemAt indexPath: IndexPath
  ) {
    sections[indexPath.section].collectionView?(
      collectionView,
      didEndDisplaying: cell,
      forItemAt: indexPath
    )
  }
}

// MARK: - UICollectionViewDataSource
extension NewTabPageViewController: UICollectionViewDataSource {
  func numberOfSections(in collectionView: UICollectionView) -> Int {
    sections.count
  }
  func collectionView(
    _ collectionView: UICollectionView,
    numberOfItemsInSection section: Int
  ) -> Int {
    sections[section].collectionView(collectionView, numberOfItemsInSection: section)
  }
  func collectionView(
    _ collectionView: UICollectionView,
    cellForItemAt indexPath: IndexPath
  ) -> UICollectionViewCell {
    sections[indexPath.section].collectionView(collectionView, cellForItemAt: indexPath)
  }
  func collectionView(
    _ collectionView: UICollectionView,
    viewForSupplementaryElementOfKind kind: String,
    at indexPath: IndexPath
  ) -> UICollectionReusableView {
    sections[indexPath.section].collectionView?(
      collectionView,
      viewForSupplementaryElementOfKind: kind,
      at: indexPath
    ) ?? UICollectionReusableView()
  }
  func collectionView(
    _ collectionView: UICollectionView,
    contextMenuConfigurationForItemAt indexPath: IndexPath,
    point: CGPoint
  ) -> UIContextMenuConfiguration? {
    sections[indexPath.section].collectionView?(
      collectionView,
      contextMenuConfigurationForItemAt: indexPath,
      point: point
    )
  }
  func collectionView(
    _ collectionView: UICollectionView,
    previewForHighlightingContextMenuWithConfiguration configuration: UIContextMenuConfiguration
  ) -> UITargetedPreview? {
    guard let indexPath = configuration.identifier as? IndexPath else {
      return nil
    }
    return sections[indexPath.section].collectionView?(
      collectionView,
      previewForHighlightingContextMenuWithConfiguration: configuration
    )
  }
  func collectionView(
    _ collectionView: UICollectionView,
    previewForDismissingContextMenuWithConfiguration configuration: UIContextMenuConfiguration
  ) -> UITargetedPreview? {
    guard let indexPath = configuration.identifier as? IndexPath else {
      return nil
    }
    return sections[indexPath.section].collectionView?(
      collectionView,
      previewForHighlightingContextMenuWithConfiguration: configuration
    )
  }
  func collectionView(
    _ collectionView: UICollectionView,
    willPerformPreviewActionForMenuWith configuration: UIContextMenuConfiguration,
    animator: UIContextMenuInteractionCommitAnimating
  ) {
    guard let indexPath = configuration.identifier as? IndexPath else {
      return
    }
    sections[indexPath.section].collectionView?(
      collectionView,
      willPerformPreviewActionForMenuWith: configuration,
      animator: animator
    )
  }
}

// MARK: - UICollectionViewDragDelegate & UICollectionViewDropDelegate

extension NewTabPageViewController: UICollectionViewDragDelegate, UICollectionViewDropDelegate {

  func collectionView(
    _ collectionView: UICollectionView,
    itemsForBeginning session: UIDragSession,
    at indexPath: IndexPath
  ) -> [UIDragItem] {
    // Check If the item that is dragged is a favourite item
    guard sections[indexPath.section] is FavoritesSectionProvider else {
      return []
    }

    let itemProvider = NSItemProvider(object: "\(indexPath)" as NSString)
    let dragItem = UIDragItem(itemProvider: itemProvider).then {
      $0.previewProvider = { () -> UIDragPreview? in
        guard let cell = collectionView.cellForItem(at: indexPath) as? FavoritesCell else {
          return nil
        }
        return UIDragPreview(view: cell.imageView)
      }
    }

    return [dragItem]
  }

  func collectionView(
    _ collectionView: UICollectionView,
    performDropWith coordinator: UICollectionViewDropCoordinator
  ) {
    guard let sourceIndexPath = coordinator.items.first?.sourceIndexPath else { return }
    let destinationIndexPath: IndexPath

    if let indexPath = coordinator.destinationIndexPath {
      destinationIndexPath = indexPath
    } else {
      let section = max(collectionView.numberOfSections - 1, 0)
      let row = collectionView.numberOfItems(inSection: section)
      destinationIndexPath = IndexPath(row: max(row - 1, 0), section: section)
    }

    guard sourceIndexPath.section == destinationIndexPath.section else { return }

    if coordinator.proposal.operation == .move {
      guard let item = coordinator.items.first else { return }
      _ = coordinator.drop(item.dragItem, toItemAt: destinationIndexPath)

      guard let favouritesSection = sections.firstIndex(where: { $0 is FavoritesSectionProvider })
      else {
        return
      }

      Favorite.reorder(
        sourceIndexPath: sourceIndexPath,
        destinationIndexPath: destinationIndexPath,
        isInteractiveDragReorder: true
      )

      UIView.performWithoutAnimation {
        self.collectionView.reloadSections(IndexSet(integer: favouritesSection))
      }

    }
  }

  func collectionView(
    _ collectionView: UICollectionView,
    dropSessionDidUpdate session: UIDropSession,
    withDestinationIndexPath destinationIndexPath: IndexPath?
  ) -> UICollectionViewDropProposal {
    guard let destinationIndexSection = destinationIndexPath?.section,
      let favouriteSection = sections[destinationIndexSection] as? FavoritesSectionProvider,
      favouriteSection.hasMoreThanOneFavouriteItems
    else {
      return .init(operation: .cancel)
    }

    return .init(operation: .move, intent: .insertAtDestinationIndexPath)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    dragPreviewParametersForItemAt indexPath: IndexPath
  ) -> UIDragPreviewParameters? {
    fetchInteractionPreviewParameters(at: indexPath)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    dropPreviewParametersForItemAt indexPath: IndexPath
  ) -> UIDragPreviewParameters? {
    fetchInteractionPreviewParameters(at: indexPath)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    dragSessionIsRestrictedToDraggingApplication session: UIDragSession
  ) -> Bool {
    return true
  }

  private func fetchInteractionPreviewParameters(at indexPath: IndexPath) -> UIDragPreviewParameters
  {
    let previewParameters = UIDragPreviewParameters().then {
      $0.backgroundColor = .clear

      if let cell = collectionView.cellForItem(at: indexPath) as? FavoritesCell {
        $0.visiblePath = UIBezierPath(roundedRect: cell.imageView.frame, cornerRadius: 8)
      }
    }

    return previewParameters
  }
}

extension NewTabPageViewController {
  private class NewTabCollectionView: UICollectionView {
    override init(frame: CGRect, collectionViewLayout layout: UICollectionViewLayout) {
      super.init(frame: frame, collectionViewLayout: layout)

      backgroundColor = .clear
      delaysContentTouches = false
      alwaysBounceVertical = true
      showsHorizontalScrollIndicator = false
      // Needed for some reason, as its not setting safe area insets while in landscape
      contentInsetAdjustmentBehavior = .always
      showsVerticalScrollIndicator = false
      // Even on light mode we use a darker background now
      indicatorStyle = .white

      // Drag should be enabled to rearrange favourite
      dragInteractionEnabled = true
    }
    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
    override func touchesShouldCancel(in view: UIView) -> Bool {
      return true
    }
  }
}

extension P3AFeatureUsage {
  fileprivate static var braveNewsFeatureUsage: Self = .init(
    name: "brave-news-usage",
    histogram: "Brave.Today.LastUsageTime",
    returningUserHistogram: "Brave.Today.NewUserReturning"
  )
}

extension P3ATimedStorage where Value == Int {
  fileprivate static var braveNewsDaysUsedStorage: Self {
    .init(name: "brave-news-days-used", lifetimeInDays: 30)
  }
  fileprivate static var braveNewsWeeklyCount: Self {
    .init(name: "brave-news-weekly-usage", lifetimeInDays: 7)
  }
  fileprivate static var braveNewsVisitedArticlesCount: Self {
    .init(name: "brave-news-weekly-clicked", lifetimeInDays: 7)
  }
  fileprivate static var newTabsCreatedStorage: Self {
    .init(name: "new-tabs-created", lifetimeInDays: 7)
  }
  fileprivate static var sponsoredNewTabsCreatedStorage: Self {
    .init(name: "sponsored-new-tabs-created", lifetimeInDays: 7)
  }
}
