// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI
import CoreData
import Data
import Shared
import BraveShared
import BraveRewards
import SnapKit

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
protocol NTPSectionProvider: NSObject, UICollectionViewDelegateFlowLayout & UICollectionViewDataSource {
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
                sectionInset = flowLayoutDelegate.collectionView?(collectionView, layout: collectionView.collectionViewLayout, insetForSectionAt: section) ?? flowLayout.sectionInset
            } else {
                sectionInset = flowLayout.sectionInset
            }
        } else {
            sectionInset = .zero
        }
        return CGSize(
            width: collectionView.bounds.width -
                collectionView.safeAreaInsets.left -
                collectionView.safeAreaInsets.right -
                sectionInset.left -
                sectionInset.right,
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
    func handleBookmarkAction(bookmark: Bookmark, action: BookmarksAction)
    func tappedDuckDuckGoCallout()
    func brandedImageCalloutActioned(_ state: BrandedImageCalloutState)
    func tappedQRCodeButton(url: URL)
}

/// The new tab page. Shows users a variety of information, including stats and
/// favourites
class NewTabPageViewController: UIViewController, Themeable {
    weak var delegate: NewTabPageDelegate?
    
    /// The modules to show on the new tab page
    private var sections: [NTPSectionProvider] = []
    
    private let layout = NewTabPageFlowLayout()
    private let collectionView: NewTabCollectionView
    private weak var tab: Tab?
    private let rewards: BraveRewards
    
    private var background: NewTabPageBackground
    private let backgroundView = NewTabPageBackgroundView()
    private let backgroundButtonsView = NewTabPageBackgroundButtonsView()
    
    private let notifications: NewTabPageNotifications
    
    init(tab: Tab,
         profile: Profile,
         dataSource: NTPDataSource,
         rewards: BraveRewards) {
        self.tab = tab
        self.rewards = rewards
        background = NewTabPageBackground(dataSource: dataSource)
        notifications = NewTabPageNotifications(rewards: rewards)
        collectionView = NewTabCollectionView(frame: .zero, collectionViewLayout: layout)
        super.init(nibName: nil, bundle: nil)
        
        sections = [
            StatsSectionProvider(),
            FavoritesSectionProvider(action: { [weak self] bookmark, action in
                self?.handleBookmarkAction(bookmark: bookmark, action: action)
            }, legacyLongPressAction: { [weak self] alertController in
                self?.present(alertController, animated: true)
            }),
            FavoritesOverflowSectionProvider(action: { [weak self] in
                self?.delegate?.focusURLBar()
            }),
            DuckDuckGoCalloutSectionProvider(profile: profile, action: { [weak self] in
                self?.delegate?.tappedDuckDuckGoCallout()
            }),
        ]
        collectionView.delegate = self
        collectionView.dataSource = self
        applyTheme(Theme.of(tab))
        
        background.changed = { [weak self] in
            self?.setupBackgroundImage()
        }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        view.addSubview(backgroundView)
        view.addSubview(collectionView)
        
        collectionView.backgroundView = backgroundButtonsView
        
        backgroundButtonsView.tappedActiveButton = { [weak self] sender in
            self?.tappedActiveBackgroundButton(sender)
        }
        
        setupBackgroundImage()
        backgroundView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
        collectionView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
        
        sections.enumerated().forEach { (index, provider) in
            provider.registerCells(to: collectionView)
            if let observableProvider = provider as? NTPObservableSectionProvider {
                observableProvider.sectionDidChange = { [weak self] in
                    UIView.performWithoutAnimation {
                        self?.collectionView.reloadSections(IndexSet(integer: index))
                    }
                }
            }
        }
    }
    
    override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()
        
        collectionView.reloadData()
        
        // Make sure that imageView has a frame calculated before we attempt
        // to use it.
        backgroundView.layoutIfNeeded()
        calculateBackgroundCenterPoints()
    }
    
    override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
        super.traitCollectionDidChange(previousTraitCollection)
        
        if #available(iOS 13.0, *),
            UITraitCollection.current.userInterfaceStyle != previousTraitCollection?.userInterfaceStyle {
            // Reload UI
            applyTheme(Theme.of(tab))
        }
    }
    
    var themeableChildren: [Themeable?]? {
        [backgroundView]
    }
    
    func applyTheme(_ theme: Theme) {
        styleChildren(theme: theme)
        collectionView.reloadData()
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        collectionView.reloadData()
    }
    
    override func viewDidAppear(_ animated: Bool) {
        super.viewDidAppear(animated)
        
        presentNotification()
    }
    
    override func viewSafeAreaInsetsDidChange() {
        super.viewSafeAreaInsetsDidChange()
        
        backgroundButtonsView.collectionViewSafeAreaInsets = view.safeAreaInsets
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
            switch background.type {
            case .regular, .withQRCode:
                hideNotification()
            case .withBrandLogo:
                // Current background is still a sponsored image so it can stay
                // visible
                break
            }
        }
    }
    
    func setupBackgroundImage() {
        collectionView.reloadData()
        
        hideVisibleSponsoredImageNotification()
        
        if let backgroundType = background.currentBackground?.type {
            switch backgroundType {
            case .regular:
                if let name = background.currentBackground?.wallpaper.credit?.name {
                    backgroundButtonsView.activeButton = .imageCredit(name)
                } else {
                    backgroundButtonsView.activeButton = .none
                }
            case .withBrandLogo(let logo):
                guard let logo = logo else { break }
                backgroundButtonsView.activeButton = .brandLogo(logo)
            case .withQRCode(_):
                backgroundButtonsView.activeButton = .QRCode
            }
        } else {
            backgroundButtonsView.activeButton = .none
        }
        
        backgroundView.gradientView.isHidden = background.backgroundImage == nil
        backgroundView.imageView.image = background.backgroundImage
        
        guard let image = backgroundView.imageView.image else {
            backgroundView.imageView.snp.removeConstraints()
            backgroundView.imageConstraints = nil
            return
        }
        
        let imageAspectRatio = image.size.width / image.size.height
        let imageView = backgroundView.imageView
        
        // Make sure it goes to the back
        imageView.snp.remakeConstraints {
            // Determines the height of the content
            // `999` priority is required for landscape, since top/bottom constraints no longer the most important
            //    using `1000` / `required` would cause constraint conflicts (with `centerY` in landscape), and
            //    using `high` is not enough either.
            $0.bottom.equalToSuperview().priority(ConstraintPriority(999))
            $0.top.equalToSuperview().priority(ConstraintPriority(999))
            
            // In portrait `top`/`bottom` is enough, however, when switching to landscape, those constraints
            //  don't force centering, so this is used as a stronger constraint to center in landscape/portrait
            let landscapeCenterConstraint = $0.top.equalTo(view.snp.centerY).priority(.high).constraint
            
            // Width of the image view is determined by the forced height constraint and the literal image ratio
            $0.width.equalTo(imageView.snp.height).multipliedBy(imageAspectRatio)
            
            // These are required constraints to avoid a bad center pushing the image out of view.
            // if a center of `-100` or `100000` is specified, these override to keep entire background covered by image.
            // The left side cannot exceed `0` (or superview's left side), otherwise whitespace will be shown on left.
            $0.left.lessThanOrEqualToSuperview()
            
            // the right side cannot drop under `width` (or superview's right side), otherwise whitespace will be shown on right.
            $0.right.greaterThanOrEqualToSuperview()
            
            // Same as left / right above but necessary for landscape y centering (to prevent overflow)
            $0.top.lessThanOrEqualToSuperview()
            $0.bottom.greaterThanOrEqualToSuperview()
            
            // If for some reason the image cannot fill full width (e.g. not a landscape image), then these constraints
            //  will fail. A constraint will be broken, since cannot keep both left and right side's pinned
            //  (due to the width multiplier being < 1
            
            // Using `high` priority so that it will not be applied / broken  if out-of-bounds.
            // Offset updated / calculated during view layout as views are not setup yet.
            let portraitCenterConstraint = $0.left.equalTo(view.snp.centerX).priority(.high).constraint
            self.backgroundView.imageConstraints = (portraitCenterConstraint, landscapeCenterConstraint)
        }
        
        // This is usually done in `viewDidLayoutSubviews`, one exception:
        // First launch, intial image loads, NTP assets is being downloaded,
        // after it downloods we replace current image with sponsored one
        // and have to recalculate center points.
        calculateBackgroundCenterPoints()
    }
    
    private func calculateBackgroundCenterPoints() {
        guard let image = backgroundView.imageView.image else { return }
        
        // Need to calculate the sizing difference between `image` and `imageView` to determine the pixel difference ratio
        let sizeRatio = backgroundView.imageView.frame.size.width / image.size.width
        let focal = background.currentBackground?.wallpaper.focalPoint
        // Center as fallback
        let x = focal?.x ?? image.size.width / 2
        let y = focal?.y ?? image.size.height / 2
        let portrait = view.frame.height > view.frame.width
        
        // Center point of image is not center point of view.
        // Take `0` for example, if specying `0`, setting centerX to 0, it is not attempting to place the left
        //  side of the image to the middle (e.g. left justifying), it is instead trying to move the image view's
        //  center to `0`, shifting the image _to_ the left, and making more of the image's right side visible.
        // Therefore specifying `0` should take the imageView's left and pinning it to view's center.
        
        // So basically the movement needs to be "inverted" (hence negation)
        // In landscape, left / right are pegged to superview
        let imageViewOffset = portrait ? sizeRatio * -x : 0
        backgroundView.imageConstraints?.portraitCenter.update(offset: imageViewOffset)
        
        // If potrait, top / bottom are just pegged to superview
        let inset = portrait ? 0 : sizeRatio * -y
        backgroundView.imageConstraints?.landscapeCenter.update(offset: inset)
    }
    
    // MARK: - Notifications
    
    private var notificationController: UIViewController?
    private var visibleNotification: NewTabPageNotifications.NotificationType?
    private var notificationShowing: Bool {
        notificationController?.parent != nil
    }
    
    private func presentNotification() {
        if PrivateBrowsingManager.shared.isPrivateBrowsing || notificationShowing {
            return
        }
        
        var isShowingSponseredImage = false
        if case .withBrandLogo(let logo) = background.currentBackground?.type, logo != nil {
            isShowingSponseredImage = true
        }
        
        guard let notification = notifications.notificationToShow(
            isShowingBackgroundImage: background.currentBackground != nil,
            isShowingSponseredImage: isShowingSponseredImage
            ) else {
                return
        }
        
        var vc: UIViewController?
        
        switch notification {
        case .brandedImages(let state):
            guard let notificationVC = NTPNotificationViewController(state: state, rewards: rewards) else { return }
            
            notificationVC.closeHandler = { [weak self] in
                self?.notificationController = nil
            }
            
            notificationVC.learnMoreHandler = { [weak self] in
                self?.delegate?.brandedImageCalloutActioned(state)
            }
            
            vc = notificationVC
        case .claimRewards:
            if !Preferences.NewTabPage.attemptToShowClaimRewardsNotification.value { return }
            
            let claimRewardsVC = ClaimRewardsNTPNotificationViewController(rewards: rewards)
            claimRewardsVC.closeHandler = { [weak self] in
                Preferences.NewTabPage.attemptToShowClaimRewardsNotification.value = false
                self?.notificationController = nil
            }
            
            vc = claimRewardsVC
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
    
    // MARK: - Actions
    
    func updateDuckDuckGoVisibility() {
        if let section = sections.firstIndex(where: { $0 is DuckDuckGoCalloutSectionProvider }) {
            collectionView.reloadSections(IndexSet(integer: section))
        }
    }
    
    private func tappedActiveBackgroundButton(_ sender: UIControl) {
        guard let background = background.currentBackground else { return }
        switch background.type {
        case .regular:
            presentImageCredit(sender)
        case .withBrandLogo(let logo):
            guard let logo = logo else { break }
            tappedSponsorButton(logo)
        case .withQRCode(let code):
            tappedQRCode(code)
        }
    }
    
    private func tappedSponsorButton(_ logo: NTPLogo) {
        UIImpactFeedbackGenerator(style: .medium).bzzt()
        delegate?.navigateToInput(logo.destinationUrl, inNewTab: false, switchingToPrivateMode: false)
    }
    
    private func tappedQRCode(_ code: String) {
        // Super referrer websites come in format https://brave.com/r/REF_CODE
        let refUrl = URL(string: "https://brave.com/")?
            .appendingPathComponent("r")
            .appendingPathComponent(code)
        
        guard let url = refUrl else { return }
        delegate?.tappedQRCodeButton(url: url)
    }
    
    private func handleBookmarkAction(bookmark: Bookmark, action: BookmarksAction) {
        delegate?.handleBookmarkAction(bookmark: bookmark, action: action)
    }
    
    private func presentImageCredit(_ button: UIControl) {
        guard let credit = background.currentBackground?.wallpaper.credit else { return }
        
        let alert = UIAlertController(title: credit.name, message: nil, preferredStyle: .actionSheet)
        
        if let creditWebsite = credit.url, let creditURL = URL(string: creditWebsite) {
            let websiteTitle = String(format: Strings.viewOn, creditURL.hostSLD.capitalizeFirstLetter)
            alert.addAction(UIAlertAction(title: websiteTitle, style: .default) { [weak self] _ in
                self?.delegate?.navigateToInput(creditWebsite, inNewTab: false, switchingToPrivateMode: false)
            })
        }
        
        alert.popoverPresentationController?.sourceView = button
        alert.popoverPresentationController?.permittedArrowDirections = [.down, .up]
        alert.addAction(UIAlertAction(title: Strings.close, style: .cancel, handler: nil))
        
        UIImpactFeedbackGenerator(style: .medium).bzzt()
        present(alert, animated: true, completion: nil)
    }
}

// MARK: - UICollectionViewDelegateFlowLayout
extension NewTabPageViewController: UICollectionViewDelegateFlowLayout {
    func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
        sections[indexPath.section].collectionView?(collectionView, didSelectItemAt: indexPath)
    }
    func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
        sections[indexPath.section].collectionView?(collectionView, layout: collectionViewLayout, sizeForItemAt: indexPath) ?? .zero
    }
    func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, insetForSectionAt section: Int) -> UIEdgeInsets {
        let sectionProvider = sections[section]
        var inset = sectionProvider.collectionView?(collectionView, layout: collectionViewLayout, insetForSectionAt: section) ?? .zero
        if sectionProvider.landscapeBehavior == .halfWidth {
            let isIphone = UIDevice.isPhone
            let isLandscape = view.frame.width > view.frame.height
            if isLandscape {
                let availableWidth = collectionView.bounds.width - collectionView.safeAreaInsets.left - collectionView.safeAreaInsets.right
                if isIphone {
                    inset.left = availableWidth / 2.0
                } else {
                    inset.right = availableWidth / 2.0
                }
            }
        }
        return inset
    }
    func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, minimumLineSpacingForSectionAt section: Int) -> CGFloat {
        sections[section].collectionView?(collectionView, layout: collectionViewLayout, minimumLineSpacingForSectionAt: section) ?? 0
    }
    func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, minimumInteritemSpacingForSectionAt section: Int) -> CGFloat {
        sections[section].collectionView?(collectionView, layout: collectionViewLayout, minimumInteritemSpacingForSectionAt: section) ?? 0
    }
}

// MARK: - UICollectionViewDataSource
extension NewTabPageViewController: UICollectionViewDataSource {
    func numberOfSections(in collectionView: UICollectionView) -> Int {
        sections.count
    }
    func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
        sections[section].collectionView(collectionView, numberOfItemsInSection: section)
    }
    func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
        let cell = sections[indexPath.section].collectionView(collectionView, cellForItemAt: indexPath)
        if let themableCell = cell as? Themeable {
            themableCell.applyTheme(Theme.of(tab))
        }
        return cell
    }
    @available(iOS 13.0, *)
    func collectionView(_ collectionView: UICollectionView, contextMenuConfigurationForItemAt indexPath: IndexPath, point: CGPoint) -> UIContextMenuConfiguration? {
        sections[indexPath.section].collectionView?(collectionView, contextMenuConfigurationForItemAt: indexPath, point: point)
    }
    @available(iOS 13.0, *)
    func collectionView(_ collectionView: UICollectionView, previewForHighlightingContextMenuWithConfiguration configuration: UIContextMenuConfiguration) -> UITargetedPreview? {
        guard let indexPath = configuration.identifier as? IndexPath else {
            return nil
        }
        return sections[indexPath.section].collectionView?(collectionView, previewForHighlightingContextMenuWithConfiguration: configuration)
    }
    @available(iOS 13.0, *)
    func collectionView(_ collectionView: UICollectionView, previewForDismissingContextMenuWithConfiguration configuration: UIContextMenuConfiguration) -> UITargetedPreview? {
        guard let indexPath = configuration.identifier as? IndexPath else {
            return nil
        }
        return sections[indexPath.section].collectionView?(collectionView, previewForHighlightingContextMenuWithConfiguration: configuration)
    }
    @available(iOS 13.0, *)
    func collectionView(_ collectionView: UICollectionView, willPerformPreviewActionForMenuWith configuration: UIContextMenuConfiguration, animator: UIContextMenuInteractionCommitAnimating) {
        guard let indexPath = configuration.identifier as? IndexPath else {
            return
        }
        sections[indexPath.section].collectionView?(collectionView, willPerformPreviewActionForMenuWith: configuration, animator: animator)
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
