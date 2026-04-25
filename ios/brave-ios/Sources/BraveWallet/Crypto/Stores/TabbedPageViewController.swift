// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Combine
import DesignSystem
import Foundation
import UIKit

private let tabBarHeight: CGFloat = 40.0

/// A container view controller which handles the navigation between any number of pages and
/// displays a scrollable tab bar above it with a list of those pages.
///
/// Navigation between pages is available by swiping back and forth or by tapping a tab directly
///
/// The title which is displayed on the tab bar are determined by `UIViewController.tabBarItem`
class TabbedPageViewController: UIViewController {
  /// A list of the pages to display
  ///
  /// Setting this automatically displays the first page
  var pages: [UIViewController] = [] {
    didSet {
      if let vc = pages.first {
        self.pageViewController.setViewControllers(
          [vc],
          direction: .forward,
          animated: false
        )
      }

      var tabsSnapshot = NSDiffableDataSourceSnapshot<Int, TabsBarView.TabItem>()
      tabsSnapshot.appendSections([0])
      tabsSnapshot.appendItems(pages.map({ .init(id: UUID(), viewController: $0) }))
      tabsBar.dataSource.apply(tabsSnapshot, animatingDifferences: false)

      titleObservers.removeAllObjects()
      for vc in pages {
        titleObservers.setObject(
          vc.publisher(for: \.navigationItem.title)
            .merge(with: vc.publisher(for: \.title))
            .map({ (vc, $0) })
            .sink { [weak self] (vc, title) in
              guard let self = self else { return }
              var snapshot = self.tabsBar.dataSource.snapshot()
              if let index = snapshot.itemIdentifiers.firstIndex(where: { $0.viewController === vc }
              ) {
                let item = snapshot.itemIdentifiers[index]
                snapshot.reloadItems([item])
                self.tabsBar.dataSource.apply(snapshot, animatingDifferences: false)
              }
            },
          forKey: vc
        )
      }

      updateTabsBarSelectionIndicator(pageIndex: 0)
    }
  }

  private var titleObservers: NSMapTable<UIViewController, AnyCancellable> = .weakToStrongObjects()

  private let tabsBar = TabsBarView()

  private let pageViewController = UIPageViewController(
    transitionStyle: .scroll,
    navigationOrientation: .horizontal,
    options: nil
  )

  private var contentOffsetObservation: NSKeyValueObservation?

  private let selectedIndexChanged: ((Int) -> Void)?

  public init(
    selectedIndexChanged: ((Int) -> Void)?
  ) {
    self.selectedIndexChanged = selectedIndexChanged
    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    view.backgroundColor = .braveBackground
    addChild(pageViewController)
    pageViewController.didMove(toParent: self)
    view.addSubview(pageViewController.view)
    view.addSubview(tabsBar)

    // Gets rid of the bottom border to appear flush with the tab bar
    navigationItem.standardAppearance = UINavigationBarAppearance().then {
      $0.configureWithDefaultBackground()
      $0.shadowColor = .clear
    }

    tabsBar.selectedTabAtIndexPath = { [unowned self] indexPath in
      moveToPage(at: indexPath)
    }
    tabsBar.boundsWidthChanged = { [unowned self] in
      updateTabsBarSelectionIndicator(pageIndex: currentIndex ?? 0)
    }

    tabsBar.snp.makeConstraints {
      $0.leading.trailing.equalToSuperview()
      $0.top.equalTo(view.safeAreaLayoutGuide)
      $0.height.equalTo(tabBarHeight)
    }

    pageViewController.view.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    pageViewController.additionalSafeAreaInsets =
      .init(top: tabBarHeight, left: 0, bottom: 0, right: 0)
    pageViewController.dataSource = self
    pageViewController.delegate = self

    if let scrollView = pageViewController.scrollView {
      contentOffsetObservation =
        scrollView.observe(\.contentOffset) { [weak self] scrollView, _ in
          self?.updateTabsBarSelectionIndicator(contentOffset: scrollView.contentOffset)
        }
    }
  }

  /// Information that is used to transition between two tabs during a scroll
  private struct PageTransitionContext {
    /// The current page index
    var currentIndex: Int
    /// The index of the page the user is scrolling to
    var targetIndex: Int
  }

  private var pageTransitionContext: PageTransitionContext?

  /// Updates the tabs bar selection indicator frame and ensures it is visible to the user
  private func updateTabsBarSelectionIndicatorFrame(x: CGFloat, width: CGFloat) {
    tabsBar.selectionIndicatorView.frame = CGRect(
      x: x,
      y: tabsBar.collectionView.bounds.height - 3,
      width: width,
      height: 3
    )
    // Ensure that when the user is scrolling near an edge, we keep the
    // selection marker in view
    let visibleRect = tabsBar.collectionView.bounds
    // Also adjust the size of the "visible" frame to allow us to peek the next
    // or previous tab
    let rectToKeepVisible = tabsBar.selectionIndicatorView.frame.insetBy(dx: -32, dy: 0)
    if !visibleRect.contains(rectToKeepVisible) {
      tabsBar.collectionView.scrollRectToVisible(rectToKeepVisible, animated: false)
    }
  }

  /// Updates the tabs bar selection indicator to display under the tab at a given page index
  private func updateTabsBarSelectionIndicator(pageIndex: Int) {
    guard pageIndex < pages.count else { return }
    view.setNeedsLayout()
    view.layoutIfNeeded()
    let layout = tabsBar.collectionView.collectionViewLayout
    let indexPath = IndexPath(item: pageIndex, section: 0)
    let currentPageFrame = layout.layoutAttributesForItem(at: indexPath)?.frame ?? .zero
    updateTabsBarSelectionIndicatorFrame(
      x: currentPageFrame.minX,
      width: currentPageFrame.width
    )
    // For accessibility purposes, mark that cell at selected. We do this here instead of at
    // the actual collectionView didSelectCell method so we correctly select cells while also
    // scrolling (this method is called at the end of each full page scroll completion)
    tabsBar.collectionView.selectItem(at: indexPath, animated: false, scrollPosition: [])
  }

  /// Updates the tabs bar selection indicator based on the content offset of the
  /// `UIPageViewController`s internal scroll view
  private func updateTabsBarSelectionIndicator(contentOffset: CGPoint) {
    guard let pageTransitionContext = pageTransitionContext else { return }
    let contentSizeWidth = tabsBar.collectionView.contentSize.width
    if contentSizeWidth.isZero {
      return
    }

    // The page process is a clamped value from -1 to 1 from the current index
    // to the target index. A negative number means user is scrolling to a
    // previous tab, and a positive number means scrolling forward
    let pageProgress: CGFloat = {
      // `UIPageViewController`'s scroll view actually always has a contentSize of 3 pages since
      // the data source just asks for each page one at a time, hence subtracing 1 page worth before
      // computing the delta
      let delta =
        (contentOffset.x - pageViewController.view.bounds.width)
        / pageViewController.view.bounds.width
      return max(-1, min(1, delta))
    }()

    // Grab the frames of the two transitioning cells
    let layout = tabsBar.collectionView.collectionViewLayout
    let currentPageFrame =
      layout.layoutAttributesForItem(
        at: .init(item: pageTransitionContext.currentIndex, section: 0)
      )?.frame ?? .zero
    let targetPageFrame =
      layout.layoutAttributesForItem(
        at: .init(item: pageTransitionContext.targetIndex, section: 0)
      )?.frame ?? .zero

    let pageDelta = abs(pageProgress)
    updateTabsBarSelectionIndicatorFrame(
      x: {
        // Interpolate origin based on the delta
        let current = currentPageFrame.origin.x
        let target = targetPageFrame.origin.x
        return current + (pageDelta * (target - current))
      }(),
      width: {
        // Interpolate width based on the delta
        let current = currentPageFrame.width
        let target = targetPageFrame.width
        return current + (pageDelta * (target - current))
      }()
    )
  }

  /// Moves to a given page and adjusts the tabs bar accordingly
  private func moveToPage(at indexPath: IndexPath) {
    guard let currentIndex = currentIndex, indexPath.item < pages.count else { return }
    let vc = pages[indexPath.item]
    let direction: UIPageViewController.NavigationDirection =
      currentIndex < indexPath.item ? .forward : .reverse
    UIViewPropertyAnimator(duration: 0.4, dampingRatio: 1.0) {
      self.updateTabsBarSelectionIndicator(pageIndex: indexPath.item)
    }.startAnimation()
    pageViewController.setViewControllers([vc], direction: direction, animated: true)
    selectedIndexChanged?(indexPath.item)
  }
}

// MARK: - UIPageViewControllerDataSource
extension TabbedPageViewController: UIPageViewControllerDataSource {
  func pageViewController(
    _ pageViewController: UIPageViewController,
    viewControllerAfter viewController: UIViewController
  ) -> UIViewController? {
    if let index = pages.firstIndex(of: viewController)?.advanced(by: 1), index < pages.count {
      return pages[index]
    }
    return nil
  }

  func pageViewController(
    _ pageViewController: UIPageViewController,
    viewControllerBefore viewController: UIViewController
  ) -> UIViewController? {
    if let index = pages.firstIndex(of: viewController)?.advanced(by: -1), index >= 0 {
      return pages[index]
    }
    return nil
  }
}

// MARK: - UIPageViewControllerDelegate
extension TabbedPageViewController: UIPageViewControllerDelegate {
  /// Obtains the current page index based on the visible view controller in the
  /// `UIPageViewController`
  private var currentIndex: Int? {
    if let currentController = pageViewController.viewControllers?.first,
      let currentIndex = pages.firstIndex(of: currentController)
    {
      return currentIndex
    }
    return nil
  }

  func pageViewController(
    _ pageViewController: UIPageViewController,
    willTransitionTo pendingViewControllers: [UIViewController]
  ) {
    guard let currentIndex = currentIndex,
      let pendingController = pendingViewControllers.first,
      let pendingIndex = pages.firstIndex(of: pendingController)
    else { return }
    self.pageTransitionContext = .init(
      currentIndex: currentIndex,
      targetIndex: pendingIndex
    )
  }

  func pageViewController(
    _ pageViewController: UIPageViewController,
    didFinishAnimating finished: Bool,
    previousViewControllers: [UIViewController],
    transitionCompleted completed: Bool
  ) {
    if !completed {
      return
    }
    self.pageTransitionContext = nil
    if let currentIndex = currentIndex {
      selectedIndexChanged?(currentIndex)
      // Update the selected tab for accessibility purposes and also in case UIPageViewController
      // ever changes how its `UIScrollView` is managed this will continue to show the user what the
      // selected tab is even if it doesn't interpolate while scrolling
      updateTabsBarSelectionIndicator(pageIndex: currentIndex)
    }
  }
}

private class TabsBarView: UIView, UICollectionViewDelegate {

  fileprivate struct TabItem: Hashable {
    var id: UUID
    var viewController: UIViewController

    var title: String? {
      viewController.title ?? viewController.navigationItem.title
    }

    func hash(into hasher: inout Hasher) {
      id.hash(into: &hasher)
    }

    static func == (lhs: Self, rhs: Self) -> Bool {
      lhs.id == rhs.id && lhs.title == rhs.title
    }
  }

  private(set) lazy var dataSource: UICollectionViewDiffableDataSource<Int, TabItem> = {
    UICollectionViewDiffableDataSource<Int, TabItem>(
      collectionView: collectionView,
      cellProvider: { cv, indexPath, item -> UICollectionViewCell? in
        guard
          let cell = cv.dequeueReusableCell(
            withReuseIdentifier: Cell.reuseIdentifier,
            for: indexPath
          ) as? Cell
        else {
          return nil
        }
        cell.title = item.title
        return cell
      }
    )
  }()

  private let shadowView = UIView().then {
    $0.backgroundColor = UIColor(white: 0.0, alpha: 0.3)
  }

  let selectionIndicatorView: BraveGradientView = .init { traitCollection in
    if traitCollection.userInterfaceStyle == .dark {
      return BraveGradient.darkGradient02
    }
    return BraveGradient.lightGradient02
  }

  let collectionView = UICollectionView(
    frame: .zero,
    collectionViewLayout: {
      let flowLayout = UICollectionViewFlowLayout()
      flowLayout.scrollDirection = .horizontal
      flowLayout.minimumInteritemSpacing = 0
      flowLayout.minimumLineSpacing = 0
      flowLayout.sectionInset = .init(top: 0, left: 16, bottom: 0, right: 16)
      flowLayout.estimatedItemSize = CGSize(width: 44, height: tabBarHeight)
      return flowLayout
    }()
  )

  var selectedTabAtIndexPath: ((IndexPath) -> Void)?
  var boundsWidthChanged: (() -> Void)?

  override var bounds: CGRect {
    didSet {
      if oldValue.width != bounds.width {
        boundsWidthChanged?()
      }
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    backgroundColor = .braveBackground

    addSubview(collectionView)
    addSubview(shadowView)
    collectionView.addSubview(selectionIndicatorView)

    collectionView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    shadowView.snp.makeConstraints {
      $0.height.equalTo(1.0 / UIScreen.main.scale)
      $0.leading.trailing.equalToSuperview()
      $0.top.equalTo(snp.bottom)
    }

    collectionView.register(Cell.self, forCellWithReuseIdentifier: Cell.reuseIdentifier)
    collectionView.dataSource = dataSource
    collectionView.delegate = self
    collectionView.backgroundColor = .clear
    collectionView.alwaysBounceVertical = false
    collectionView.alwaysBounceHorizontal = true
    collectionView.showsHorizontalScrollIndicator = false
    collectionView.isDirectionalLockEnabled = true
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  // MARK: - UICollectionViewDelegate

  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    selectedTabAtIndexPath?(indexPath)
  }

  // MARK: -

  private class Cell: UICollectionViewCell {
    static let reuseIdentifier = "tabCell"

    var title: String? {
      didSet {
        label.text = title?.uppercased()

        // Accessibility
        accessibilityLabel = title
        largeContentTitle = title
      }
    }

    let label = UILabel().then {
      $0.font = .boldSystemFont(ofSize: 15)
      $0.textAlignment = .center
    }

    override init(frame: CGRect) {
      super.init(frame: frame)

      contentView.addSubview(label)

      label.snp.makeConstraints {
        $0.leading.trailing.equalTo(contentView).inset(12)
        $0.centerY.equalTo(contentView)
      }

      isAccessibilityElement = true
      accessibilityTraits = [.staticText, .button]
      // Since we aren't going to support dynamic type on the tab titles, we will adopt a large
      // content viewer for the tabs
      showsLargeContentViewer = true
      addInteraction(UILargeContentViewerInteraction())
    }

    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
  }
}

extension UIPageViewController {
  /// Returns the internal `UIScrollView` created by the page controller
  fileprivate var scrollView: UIScrollView? {
    view.subviews.compactMap({ $0 as? UIScrollView }).first
  }
}
