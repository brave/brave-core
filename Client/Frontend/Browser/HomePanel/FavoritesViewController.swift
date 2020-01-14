/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import BraveShared
import Storage
import Deferred
import Data
import SnapKit

private let log = Logger.browserLogger

protocol TopSitesDelegate: AnyObject {
    func didSelect(input: String)
    func didTapDuckDuckGoCallout()
    func didTapShowMoreFavorites()
}

class FavoritesViewController: UIViewController, Themeable {
    private struct UI {
        static let statsHeight: CGFloat = 110.0
        static let statsBottomMargin: CGFloat = 5
        static let searchEngineCalloutPadding: CGFloat = 120.0
    }
    
    weak var delegate: TopSitesDelegate?
    
    // MARK: - Favorites collection view properties
    private (set) internal lazy var collection: UICollectionView = {
        let layout = UICollectionViewFlowLayout()
        layout.minimumInteritemSpacing = 0
        layout.minimumLineSpacing = 6
        
        let view = UICollectionView(frame: self.view.frame, collectionViewLayout: layout).then {
            $0.backgroundColor = .clear
            $0.delegate = self
        
            let cellIdentifier = FavoriteCell.identifier
            $0.register(FavoriteCell.self, forCellWithReuseIdentifier: cellIdentifier)
            $0.keyboardDismissMode = .onDrag
            $0.alwaysBounceVertical = true
            $0.accessibilityIdentifier = "Top Sites View"
            // Entire site panel, including the stats view insets
            $0.contentInset = UIEdgeInsets(top: UI.statsHeight, left: 0, bottom: 0, right: 0)
        }
        return view
    }()
    private let dataSource: FavoritesDataSource
    
    private let braveShieldStatsView = BraveShieldStatsView(frame: CGRect.zero).then {
        $0.autoresizingMask = [.flexibleWidth]
    }
    
    private lazy var favoritesOverflowButton = RoundInterfaceView().then {
        let blur = UIVisualEffectView(effect: UIBlurEffect(style: .light))
        let button = UIButton(type: .system).then {
            $0.setTitle(Strings.newTabPageShowMoreFavorites, for: .normal)
            $0.appearanceTextColor = .white
            $0.titleLabel?.font = UIFont.systemFont(ofSize: 12.0, weight: .medium)
            $0.addTarget(self, action: #selector(showFavorites), for: .touchUpInside)
        }
        
        $0.clipsToBounds = true
        
        $0.addSubview(blur)
        $0.addSubview(button)
        
        blur.snp.makeConstraints { $0.edges.equalToSuperview() }
        button.snp.makeConstraints { $0.edges.equalToSuperview() }
    }
    
    // Needs to be own variable in order to dynamically set title contents
    private lazy var imageCreditInternalButton = UIButton(type: .system).then {
        $0.appearanceTextColor = .white
        $0.titleLabel?.font = UIFont.systemFont(ofSize: 12.0, weight: .medium)
        $0.addTarget(self, action: #selector(showImageCredit), for: .touchUpInside)
    }
    
    private lazy var imageCreditButton = UIView().then {
        let blur = UIVisualEffectView(effect: UIBlurEffect(style: .light))
        blur.contentView.backgroundColor = UIColor.black.withAlphaComponent(0.4)
        $0.clipsToBounds = true
        $0.layer.cornerRadius = 4
        
        $0.addSubview(blur)
        $0.addSubview(imageCreditInternalButton)
        
        blur.snp.makeConstraints { $0.edges.equalToSuperview() }
        imageCreditInternalButton.snp.makeConstraints {
            $0.top.bottom.equalToSuperview()
            let padding = 10
            $0.left.equalToSuperview().offset(padding)
            $0.right.equalToSuperview().inset(padding)
        }
    }
    
    private let ddgLogo = UIImageView(image: #imageLiteral(resourceName: "duckduckgo"))
    
    private let ddgLabel = UILabel().then {
        $0.numberOfLines = 0
        $0.textColor = BraveUX.greyD
        $0.font = UIFont.systemFont(ofSize: 14, weight: UIFont.Weight.regular)
        $0.text = Strings.DDGPromotion
    }
    
    private lazy var ddgButton = RoundInterfaceView().then {
        let blur = UIVisualEffectView(effect: UIBlurEffect(style: .dark))
        let actualButton = UIButton(type: .system).then {
            $0.addTarget(self, action: #selector(showDDGCallout), for: .touchUpInside)
        }
        $0.clipsToBounds = true
        
        $0.addSubview(blur)
        $0.addSubview(actualButton)
        
        blur.snp.makeConstraints { $0.edges.equalToSuperview() }
        actualButton.snp.makeConstraints { $0.edges.equalToSuperview() }
    }
    
    @objc private func showDDGCallout() {
        delegate?.didTapDuckDuckGoCallout()
    }
    
    @objc private func showFavorites() {
        delegate?.didTapShowMoreFavorites()
    }
    
    // MARK: - Init/lifecycle
    
    private var backgroundViewInfo: (imageView: UIImageView, portraitCenterConstraint: Constraint)?
    private var backgroundImage = BackgroundImage()
    
    private let profile: Profile
    
    init(profile: Profile, dataSource: FavoritesDataSource = FavoritesDataSource()) {
        self.profile = profile
        self.dataSource = dataSource
        
        super.init(nibName: nil, bundle: nil)
        NotificationCenter.default.do {
            $0.addObserver(self, selector: #selector(existingUserTopSitesConversion), 
                           name: .topSitesConversion, object: nil)
            $0.addObserver(self, selector: #selector(privateBrowsingModeChanged), 
                           name: .privacyModeChanged, object: nil)
        }
    }
    
    @objc func existingUserTopSitesConversion() {
        dataSource.refetch()
        collection.reloadData()
    }
    
    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    deinit {
        NotificationCenter.default.do {
            $0.removeObserver(self, name: .topSitesConversion, object: nil)
            $0.removeObserver(self, name: .privacyModeChanged, object: nil)
        }
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        view.clipsToBounds = true
        
        setupBackgroundImage()
        // Setup gradient regardless of background image, can internalize to setup background image if only wanted for images.
        view.layer.addSublayer(gradientOverlay())
        
        let longPressGesture = UILongPressGestureRecognizer(target: self, action: #selector(handleLongGesture(gesture:)))
        collection.addGestureRecognizer(longPressGesture)
        
        let tapGesture = UITapGestureRecognizer(target: self, action: #selector(handleTapGesture(gesture:)))
        // Adding a clear background for tap gestures, otherwise impossible to tap other buttons / icons on NTP
        let background = UIView()
        background.backgroundColor = .clear
        background.addGestureRecognizer(tapGesture)
        collection.backgroundView = background
        
        view.addSubview(collection)
        collection.dataSource = dataSource
        dataSource.collectionView = collection
        
        dataSource.favoriteDeletedHandler = { [weak self] in
            self?.favoritesOverflowButton.isHidden = self?.dataSource.hasOverflow == false
        }
        
        // Could setup as section header but would need to use flow layout,
        // Auto-layout subview within collection doesn't work properly,
        // Quick-and-dirty layout here.
        var statsViewFrame: CGRect = braveShieldStatsView.frame
        statsViewFrame.origin.x = 20
        // Offset the stats view from the inset set above
        statsViewFrame.origin.y = -(UI.statsHeight + UI.statsBottomMargin)
        statsViewFrame.size.width = collection.frame.width - statsViewFrame.minX * 2
        statsViewFrame.size.height = UI.statsHeight
        braveShieldStatsView.frame = statsViewFrame
        
        collection.addSubview(braveShieldStatsView)
        collection.addSubview(favoritesOverflowButton)
        collection.addSubview(ddgButton)
        collection.addSubview(imageCreditButton)
        
        ddgButton.addSubview(ddgLogo)
        ddgButton.addSubview(ddgLabel)
        
        makeConstraints()
        
        Preferences.NewTabPage.backgroundImages.observe(from: self)
        Preferences.NewTabPage.backgroundSponsoredImages.observe(from: self)
        
        // Doens't this get called twice?
        collectionContentSizeObservation = collection.observe(\.contentSize, options: [.new, .initial]) { [weak self] _, _ in
            self?.updateDuckDuckGoButtonLayout()
        }
        updateDuckDuckGoVisibility()
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        // Need to reload data after modals are closed for potential orientation change
        // e.g. if in landscape, open portrait modal, close, the layout attempt to access an invalid indexpath
        collection.reloadData()
    }
    
    private var collectionContentSizeObservation: NSKeyValueObservation?
    
    override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()
        
        // This makes collection view layout to recalculate its cell size.
        collection.collectionViewLayout.invalidateLayout()
        favoritesOverflowButton.isHidden = !dataSource.hasOverflow
        collection.reloadSections(IndexSet(arrayLiteral: 0))
        
        if let backgroundImageView = backgroundViewInfo?.imageView, let image = backgroundImageView.image {
            // Need to calculate the sizing difference between `image` and `imageView` to determine the pixel difference ratio
            let sizeRatio = backgroundImageView.frame.size.width / image.size.width
            
            // Center point of image is not center point of view.
            // Take `0` for example, if specying `0`, setting centerX to 0, it is not attempting to place the left
            //  side of the image to the middle (e.g. left justifying), it is instead trying to move the image view's
            //  center to `0`, shifting the image _to_ the left, and making more of the image's right side visible.
            // Therefore specifying `0` should take the imageView's left and pinning it to view's center.
            
            // So basically the movement needs to be "inverted" (hence negation)
            let imageViewOffset = sizeRatio * -(backgroundImage.info?.center ?? 0)
            backgroundViewInfo?.portraitCenterConstraint.update(offset: imageViewOffset)
        }
    }
    
    private func updateDuckDuckGoButtonLayout() {
        let size = ddgButton.systemLayoutSizeFitting(UIView.layoutFittingExpandedSize)
        ddgButton.frame = CGRect(
            x: ceil((collection.bounds.width - size.width) / 2.0),
            y: collection.contentSize.height + UI.searchEngineCalloutPadding,
            width: size.width,
            height: size.height
        )
    }
    
    /// Handles long press gesture for UICollectionView cells reorder.
    @objc func handleLongGesture(gesture: UILongPressGestureRecognizer) {
        switch gesture.state {
        case .began:
            guard let selectedIndexPath = collection.indexPathForItem(at: gesture.location(in: collection)) else {
                handleLongGestureForBackground(gesture: gesture)
                break
            }
            
            dataSource.isEditing = true
            collection.beginInteractiveMovementForItem(at: selectedIndexPath)
        case .changed:
            collection.updateInteractiveMovementTargetPosition(gesture.location(in: gesture.view!))
        case .ended:
            collection.endInteractiveMovement()
        default:
            collection.cancelInteractiveMovement()
        }
    }
    
    @objc func handleTapGesture(gesture: UITapGestureRecognizer) {
        // Tap gesture only actionable with sponsored images.
        if gesture.state == .ended && backgroundImage.info?.isSponsored == true {
            showImageCredit()
        }
    }
    
    /// Handles long press gesture for background credit
    func handleLongGestureForBackground(gesture: UILongPressGestureRecognizer) {
        if gesture.state != .began {
            return
        }
        showImageCredit()
    }
    
    @objc fileprivate func showImageCredit() {
        guard let credit = backgroundImage.info?.credit else {
            // No gesture action of no credit available
            return
        }
        
        let alert = UIAlertController(title: credit.name, message: nil, preferredStyle: .actionSheet)
        
        if let creditWebsite = credit.url, let creditURL = URL(string: creditWebsite) {
            let websiteTitle = String(format: Strings.viewOn, creditURL.hostSLD.capitalizeFirstLetter)
            alert.addAction(UIAlertAction(title: websiteTitle, style: .default) { [weak self] _ in
                self?.delegate?.didSelect(input: creditWebsite)
            })
        }
        
        alert.popoverPresentationController?.sourceView = view
        alert.popoverPresentationController?.sourceRect = CGRect(origin: view.center, size: .zero)
        alert.popoverPresentationController?.permittedArrowDirections = [.down, .up]
        alert.addAction(UIAlertAction(title: Strings.close, style: .cancel, handler: nil))
        
        present(alert, animated: true, completion: nil)
    }
    
    // MARK: - Constraints setup
    fileprivate func makeConstraints() {
        collection.snp.makeConstraints { make in
            make.left.right.equalTo(self.view.safeAreaLayoutGuide)
            make.top.bottom.equalTo(self.view)
        }
        
        ddgLogo.snp.makeConstraints { make in
            make.top.left.bottom.equalTo(0)
            make.size.equalTo(38)
        }
        
        ddgLabel.snp.makeConstraints { make in
            make.top.bottom.equalTo(0)
            make.right.equalToSuperview().offset(-5)
            make.left.equalTo(self.ddgLogo.snp.right).offset(5)
            make.width.equalTo(180)
            make.centerY.equalTo(self.ddgLogo)
        }
        
        favoritesOverflowButton.snp.makeConstraints {
            $0.centerX.equalToSuperview()
            $0.bottom.equalTo(ddgButton.snp.top).offset(-90)
            $0.height.equalTo(24)
            $0.width.equalTo(84)
        }
        
        imageCreditButton.snp.makeConstraints {
            let borderPadding = 20
            $0.bottom.equalTo(self.view.snp.bottom).inset(borderPadding)
            $0.left.equalToSuperview().offset(borderPadding)
            $0.height.equalTo(24)
            // Width and therefore, right constraint is determined by the actual button inside of this view
            //  button is resized from text content, and this superview is pinned to that width.
        }
    }
    
    // MARK: - Private browsing modde
    @objc func privateBrowsingModeChanged() {
        updateDuckDuckGoVisibility()
    }
    
    var themeableChildren: [Themeable?]? {
        return [braveShieldStatsView]
    }
    
    func applyTheme(_ theme: Theme) {
        styleChildren(theme: theme)
       
        view.backgroundColor = theme.colors.home
    }
    
    override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
        super.traitCollectionDidChange(previousTraitCollection)
        
        collection.collectionViewLayout.invalidateLayout()
    }
    
    private func setupImageCredit() {
        var hideImageCredit = true
        defer {
            imageCreditButton.isHidden = hideImageCredit
        }
        
        guard let info = backgroundImage.info, let name = info.credit?.name else { return }
        
        hideImageCredit = info.isSponsored
        let photoByText = String(format: Strings.photoBy, name)
        imageCreditInternalButton.setTitle(photoByText, for: .normal)
    }
    
    private func setupBackgroundImage() {
        guard var background = backgroundImage.info,
            let image = background.image else {
                imageCreditButton.isHidden = true
                return
        }
        
        setupImageCredit()
        let imageAspectRatio = image.size.width / image.size.height
        let imageView = UIImageView(image: image)
        
        imageView.contentMode = UIImageView.ContentMode.scaleAspectFit
        // Make sure it goes to the back
        view.insertSubview(imageView, at: 0)
        
        imageView.translatesAutoresizingMaskIntoConstraints = false
        imageView.snp.makeConstraints {
            
            // Determines the height of the content
            // `999` priority is required for landscape, since top/bottom constraints no longer the most important
            //    using `1000` / `required` would cause constraint conflicts (with `centerY` in landscape), and
            //    using `high` is not enough either.
            $0.top.bottom.equalToSuperview().priority(ConstraintPriority(999))
            
            // In portrait `top`/`bottom` is enough, however, when switching to landscape, those constraints
            //  don't force centering, so this is used as a stronger constraint to center in landscape/portrait
            $0.centerY.equalToSuperview()
            
            // Width of the image view is determined by the forced height constraint and the literal image ratio
            $0.width.equalTo(imageView.snp.height).multipliedBy(imageAspectRatio)
            
            // These are required constraints to avoid a bad center pushing the image out of view.
            // if a center of `-100` or `100000` is specified, these override to keep entire background covered by image.
            // The left side cannot exceed `0` (or superview's left side), otherwise whitespace will be shown on left.
            $0.left.lessThanOrEqualToSuperview()
            
            // the right side cannot drop under `width` (or superview's right side), otherwise whitespace will be shown on right.
            $0.right.greaterThanOrEqualToSuperview()
            
            // If for some reason the image cannot fill full width (e.g. not a landscape image), then these constraints
            //  will fail. A constraint will be broken, since cannot keep both left and right side's pinned
            //  (due to the width multiplier being < 1
            
            // Using `high` priority so that it will not be applied / broken  if out-of-bounds.
            // Offset updated / calculated during view layout as views are not setup yet.
            let backgroundConstraint = $0.left.equalTo(view.snp.centerX).priority(ConstraintPriority.high).constraint
            self.backgroundViewInfo = (imageView, backgroundConstraint)
        }
    }
    
    fileprivate func resetBackground() {
        self.backgroundViewInfo?.imageView.removeFromSuperview()
        self.backgroundViewInfo = nil
        
        // Flush background logic, this handles preference adjustments for us, so will update necessary, needed info.
        backgroundImage = BackgroundImage()
        
        setupBackgroundImage()
    }
    
    fileprivate func gradientOverlay() -> CAGradientLayer {
        
        // Fades from half-black to transparent
        let colorTop = UIColor(white: 0.0, alpha: 0.5).cgColor
        let colorBottom = UIColor(white: 0.0, alpha: 0.0).cgColor
        
        let gl = CAGradientLayer()
        gl.colors = [colorTop, colorBottom]
        
        // Gradient cover percentage
        gl.locations = [0.0, 0.5]
        
        // Making a squrare to handle rotation events
        let maxSide = max(view.bounds.height, view.bounds.width)
        gl.frame = CGRect(size: CGSize(width: maxSide, height: maxSide))
        
        return gl
    }
    
    // MARK: DuckDuckGo
    
    func shouldShowDuckDuckGoCallout() -> Bool {
        let isSearchEngineSet = profile.searchEngines.defaultEngine(forType: .privateMode).shortName == OpenSearchEngine.EngineNames.duckDuckGo
        let isPrivateBrowsing = PrivateBrowsingManager.shared.isPrivateBrowsing
        let shouldShowPromo = SearchEngines.shouldShowDuckDuckGoPromo
        return isPrivateBrowsing && !isSearchEngineSet && shouldShowPromo
    }
    
    func updateDuckDuckGoVisibility() {
        let isVisible = shouldShowDuckDuckGoCallout()
        let heightOfCallout = ddgButton.systemLayoutSizeFitting(UIView.layoutFittingExpandedSize).height + (UI.searchEngineCalloutPadding * 2.0)
        collection.contentInset.bottom = isVisible ? heightOfCallout : 0
        ddgButton.isHidden = !isVisible
    }
}

// MARK: - Delegates
extension FavoritesViewController: UICollectionViewDelegateFlowLayout {
    func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
        let fav = dataSource.favoriteBookmark(at: indexPath)
        
        guard let urlString = fav?.url else { return }
        
        delegate?.didSelect(input: urlString)
    }
    
    func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
        let width = collection.frame.width
        let padding: CGFloat = traitCollection.horizontalSizeClass == .compact ? 6 : 20
        
        let cellWidth = floor(width - padding) / CGFloat(dataSource.columnsPerRow)
        // The tile's height is determined the aspect ratio of the thumbnails width. We also take into account
        // some padding between the title and the image.
        let cellHeight = floor(cellWidth / (CGFloat(FavoriteCell.imageAspectRatio) - 0.1))
        
        return CGSize(width: cellWidth, height: cellHeight)
    }
    
    func collectionView(_ collectionView: UICollectionView, willDisplay cell: UICollectionViewCell, forItemAt indexPath: IndexPath) {
        guard let favoriteCell = cell as? FavoriteCell else { return }
        favoriteCell.delegate = self
    }
}

extension FavoritesViewController: FavoriteCellDelegate {
    func editFavorite(_ favoriteCell: FavoriteCell) {
        guard let indexPath = collection.indexPath(for: favoriteCell),
            let fav = dataSource.frc?.fetchedObjects?[indexPath.item] else { return }
        
        let actionSheet = UIAlertController(title: fav.displayTitle, message: nil, preferredStyle: .actionSheet)
        
        let deleteAction = UIAlertAction(title: Strings.removeFavorite, style: .destructive) { _ in
            fav.delete()
            
            // Remove cached icon.
            if let urlString = fav.url, let url = URL(string: urlString) {
                ImageCache.shared.remove(url, type: .square)
            }
            
            self.dataSource.isEditing = false
        }
        
        let editAction = UIAlertAction(title: Strings.editFavorite, style: .default) { _ in
            guard let title = fav.displayTitle, let urlString = fav.url else { return }
            
            let editPopup = UIAlertController.userTextInputAlert(title: Strings.editBookmark, message: urlString,
                                                                 startingText: title, startingText2: fav.url,
                                                                 placeholder2: urlString,
                                                                 keyboardType2: .URL) { callbackTitle, callbackUrl in
                                                                    if let cTitle = callbackTitle, !cTitle.isEmpty, let cUrl = callbackUrl, !cUrl.isEmpty {
                                                                        if URL(string: cUrl) != nil {
                                                                            fav.update(customTitle: cTitle, url: cUrl)
                                                                        }
                                                                    }
                                                                    self.dataSource.isEditing = false
            }
            
            self.present(editPopup, animated: true)
        }
        
        let cancelAction = UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil)
        
        actionSheet.addAction(editAction)
        actionSheet.addAction(deleteAction)
        actionSheet.addAction(cancelAction)
        
        if UIDevice.current.userInterfaceIdiom == .pad {
            actionSheet.popoverPresentationController?.permittedArrowDirections = .any
            actionSheet.popoverPresentationController?.sourceView = favoriteCell
            actionSheet.popoverPresentationController?.sourceRect = favoriteCell.bounds
            present(actionSheet, animated: true)
        } else {
            present(actionSheet, animated: true) {
                self.dataSource.isEditing = false
            }
        }
    }
}

extension FavoritesViewController: PreferencesObserver {
    func preferencesDidChange(for key: String) {
        self.resetBackground()
    }
}

extension CGSize {
    public func widthLargerOrEqualThanHalfIPad() -> Bool {
        let halfIPadSize: CGFloat = 507
        return width >= halfIPadSize
    }
}
