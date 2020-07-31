/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared

private let log = Logger.browserLogger

class TrayToBrowserAnimator: NSObject, UIViewControllerAnimatedTransitioning {
    func animateTransition(using transitionContext: UIViewControllerContextTransitioning) {
        if let bvc = transitionContext.viewController(forKey: .to) as? BrowserViewController,
           let tabTray = transitionContext.viewController(forKey: .from) as? TabTrayController {
            transitionFromTray(tabTray, toBrowser: bvc, usingContext: transitionContext)
        }
    }

    func transitionDuration(using transitionContext: UIViewControllerContextTransitioning?) -> TimeInterval {
        return 0.4
    }
}

private extension TrayToBrowserAnimator {
    func transitionFromTray(_ tabTray: TabTrayController, toBrowser bvc: BrowserViewController, usingContext transitionContext: UIViewControllerContextTransitioning) {
        let container = transitionContext.containerView

        let tabManager = bvc.tabManager
        let displayedTabs = tabManager.tabsForCurrentMode
        
        guard let selectedTab = bvc.tabManager.selectedTab, let expandFromIndex = displayedTabs.firstIndex(of: selectedTab) else {
            log.error("No tab selected for transition.")
            transitionContext.completeTransition(true)
            return
        }
        bvc.view.frame = transitionContext.finalFrame(for: bvc)

        // Hide browser components
        bvc.toggleSnackBarVisibility(show: false)
        toggleWebViewVisibility(false, usingTabManager: bvc.tabManager)
        bvc.activeNewTabPageViewController?.view.isHidden = true
        selectedTab.newTabPageViewController?.view.isHidden = true
        bvc.webViewContainerBackdrop.isHidden = true
        bvc.statusBarOverlay.isHidden = false
        if let url = selectedTab.url, !url.isReaderModeURL {
            bvc.hideReaderModeBar(animated: false)
        }

        // Take a snapshot of the collection view that we can scale/fade out. We don't need to wait for screen updates since it's already rendered on the screen
        let tabCollectionViewSnapshot = tabTray.collectionView.snapshotView(afterScreenUpdates: false)!
        tabTray.collectionView.alpha = 0
        tabCollectionViewSnapshot.frame = tabTray.collectionView.frame
        container.addSubview(tabCollectionViewSnapshot)

        // Create a fake cell to use for the upscaling animation
        let startingFrame = calculateCollapsedCellFrameUsingCollectionView(tabTray.collectionView, atIndex: expandFromIndex)
        let cell = createTransitionCellFromTab(bvc.tabManager.selectedTab, withFrame: startingFrame)
        cell.backgroundHolder.layer.cornerRadius = 0

        container.insertSubview(bvc.view, aboveSubview: tabCollectionViewSnapshot)
        container.insertSubview(cell, aboveSubview: bvc.view)

        // Flush any pending layout/animation code in preperation of the animation call
        container.layoutIfNeeded()

        let finalFrame = calculateExpandedCellFrameFromBVC(bvc)
        bvc.footer.alpha = shouldDisplayFooterForBVC(bvc) ? 1 : 0
        bvc.topToolbar.isTransitioning = true

        // Re-calculate the starting transforms for header/footer views in case we switch orientation
        resetTransformsForViews([bvc.header, bvc.readerModeBar, bvc.footer])
        transformHeaderFooterForBVC(bvc, toFrame: startingFrame, container: container)

        UIView.animate(withDuration: self.transitionDuration(using: transitionContext),
            delay: 0, usingSpringWithDamping: 1,
            initialSpringVelocity: 0,
            options: [],
            animations: {
            // Scale up the cell and reset the transforms for the header/footers
            cell.frame = finalFrame
            container.layoutIfNeeded()
            cell.titleBackgroundView.transform = CGAffineTransform(translationX: 0, y: -cell.titleBackgroundView.frame.height)
            cell.layer.borderWidth = 0.0

            bvc.tabTrayDidDismiss(tabTray)
            tabTray.navigationController?.setNeedsStatusBarAppearanceUpdate()
            tabTray.toolbar.transform = CGAffineTransform(translationX: 0, y: UIConstants.bottomToolbarHeight)
            tabCollectionViewSnapshot.transform = CGAffineTransform(scaleX: 0.9, y: 0.9)
            tabCollectionViewSnapshot.alpha = 0
        }, completion: { finished in
            // Remove any of the views we used for the animation
            cell.removeFromSuperview()
            tabCollectionViewSnapshot.removeFromSuperview()
            bvc.footer.alpha = 1
            bvc.toggleSnackBarVisibility(show: true)
            toggleWebViewVisibility(true, usingTabManager: bvc.tabManager)
            bvc.webViewContainerBackdrop.isHidden = false
            selectedTab.newTabPageViewController?.view.isHidden = false
            bvc.topToolbar.isTransitioning = false
            bvc.updateTabsBarVisibility()
            transitionContext.completeTransition(true)
            
            self.WKWebViewPDFNotRenderingBugFix(for: bvc)
        })
    }
    
    /// Fixes a Bug in iOS 12.2 where:
    /// 1. Have Navigation Controller
    /// 2. Set a ViewController with a WKWebView as root controller
    /// 3. Push any other controller
    /// 4. Dismiss the controller
    /// 5. Verify that PDF not rendering.
    /// 6. Present any controller and dismiss it
    /// 7. Verify PDF re-rendered.
    /// Note: This only happens when a UINavigationController ".push".
    ///       It does not happen for `.present`
    
    /// Bug is present in FireFox iOS: https://stackoverflow.com/questions/52735158/wkwebview-shows-gray-background-and-pdf-content-gets-invisible-on-viewcontroller
    /// Although, they solve it differently..
    /// Confirmed by WebKit that it's an iOS bug: https://bugs.webkit.org/show_bug.cgi?id=193281
    private func WKWebViewPDFNotRenderingBugFix(for controller: BrowserViewController) {
        guard let mimeType = controller.tabManager.selectedTab?.mimeType, !mimeType.isKindOfHTML else {
            return
        }
        
        if mimeType.lowercased().contains("pdf") {
            let fakeController = UIViewController()
            if let navController = controller.navigationController {
                navController.present(fakeController, animated: false, completion: {
                    fakeController.dismiss(animated: false, completion: nil)
                })
            }
        }
    }
}

class BrowserToTrayAnimator: NSObject, UIViewControllerAnimatedTransitioning {
    func animateTransition(using transitionContext: UIViewControllerContextTransitioning) {
        if let bvc = transitionContext.viewController(forKey: .from) as? BrowserViewController,
           let tabTray = transitionContext.viewController(forKey: .to) as? TabTrayController {
            transitionFromBrowser(bvc, toTabTray: tabTray, usingContext: transitionContext)
        }
    }

    func transitionDuration(using transitionContext: UIViewControllerContextTransitioning?) -> TimeInterval {
        return 0.4
    }
}

private extension BrowserToTrayAnimator {
    func transitionFromBrowser(_ bvc: BrowserViewController, toTabTray tabTray: TabTrayController, usingContext transitionContext: UIViewControllerContextTransitioning) {

        let container = transitionContext.containerView
        let tabManager = bvc.tabManager
        let displayedTabs = tabManager.tabsForCurrentMode
        guard let selectedTab = bvc.tabManager.selectedTab, let scrollToIndex = displayedTabs.firstIndex(of: selectedTab) else {
            log.error("No tab selected for transition.")
            transitionContext.completeTransition(true)
            return
        }

        tabTray.view.frame = transitionContext.finalFrame(for: tabTray)

        // Insert tab tray below the browser and force a layout so the collection view can get it's frame right
        container.insertSubview(tabTray.view, belowSubview: bvc.view)

        // Force subview layout on the collection view so we can calculate the correct end frame for the animation
        tabTray.view.layoutSubviews()

        tabTray.collectionView.scrollToItem(at: IndexPath(item: scrollToIndex, section: 0), at: .centeredVertically, animated: false)

        // Build a tab cell that we will use to animate the scaling of the browser to the tab
        let expandedFrame = calculateExpandedCellFrameFromBVC(bvc)
        let cell = createTransitionCellFromTab(bvc.tabManager.selectedTab, withFrame: expandedFrame)
        cell.backgroundHolder.layer.cornerRadius = TabTrayControllerUX.cornerRadius

        // Take a snapshot of the collection view to perform the scaling/alpha effect
        let tabCollectionViewSnapshot = tabTray.collectionView.snapshotView(afterScreenUpdates: true)!
        tabCollectionViewSnapshot.frame = tabTray.collectionView.frame
        tabCollectionViewSnapshot.transform = CGAffineTransform(scaleX: 0.9, y: 0.9)
        tabCollectionViewSnapshot.alpha = 0
        tabTray.view.insertSubview(tabCollectionViewSnapshot, belowSubview: tabTray.toolbar)

        if let toast = bvc.clipboardBarDisplayHandler?.clipboardToast {
            toast.removeFromSuperview()
        }
        
        container.addSubview(cell)
        cell.layoutIfNeeded()
        cell.titleBackgroundView.transform = CGAffineTransform(translationX: 0, y: -cell.titleBackgroundView.frame.size.height)

        // Hide views we don't want to show during the animation in the BVC
        bvc.tabManager.selectedTab?.newTabPageViewController?.view.isHidden = true
        bvc.statusBarOverlay.isHidden = true
        bvc.toggleSnackBarVisibility(show: false)
        toggleWebViewVisibility(false, usingTabManager: bvc.tabManager)
        bvc.topToolbar.isTransitioning = true

        // Since we are hiding the collection view and the snapshot API takes the snapshot after the next screen update,
        // the screenshot ends up being blank unless we set the collection view hidden after the screen update happens.
        // To work around this, we dispatch the setting of collection view to hidden after the screen update is completed.

        DispatchQueue.main.async {
            tabTray.collectionView.isHidden = true
            let finalFrame = calculateCollapsedCellFrameUsingCollectionView(tabTray.collectionView,
                atIndex: scrollToIndex)
            tabTray.toolbar.transform = CGAffineTransform(translationX: 0, y: UIConstants.bottomToolbarHeight)

            UIView.animate(withDuration: self.transitionDuration(using: transitionContext),
                delay: 0, usingSpringWithDamping: 1,
                initialSpringVelocity: 0,
                options: [],
                animations: {
                cell.frame = finalFrame
                cell.titleBackgroundView.transform = .identity
                cell.layoutIfNeeded()
                tabTray.navigationController?.setNeedsStatusBarAppearanceUpdate()
                    
                cell.layer.borderWidth = TabTrayControllerUX.defaultBorderWidth
                
                transformHeaderFooterForBVC(bvc, toFrame: finalFrame, container: container)

                bvc.topToolbar.updateAlphaForSubviews(0)
                bvc.footer.alpha = 0
                tabCollectionViewSnapshot.alpha = 1

                tabTray.toolbar.transform = .identity
                resetTransformsForViews([tabCollectionViewSnapshot])
            }, completion: { finished in
                // Remove any of the views we used for the animation
                cell.removeFromSuperview()
                tabCollectionViewSnapshot.removeFromSuperview()
                tabTray.collectionView.isHidden = false

                bvc.toggleSnackBarVisibility(show: true)
                toggleWebViewVisibility(true, usingTabManager: bvc.tabManager)
                bvc.tabManager.selectedTab?.newTabPageViewController?.view.isHidden = false

                resetTransformsForViews([bvc.header, bvc.readerModeBar, bvc.footer])
                bvc.topToolbar.isTransitioning = false
                transitionContext.completeTransition(true)
            })
        }
    }
}

private func transformHeaderFooterForBVC(_ bvc: BrowserViewController, toFrame finalFrame: CGRect, container: UIView) {
    let footerForTransform = footerTransform(bvc.footer.frame, toFrame: finalFrame, container: container)
    let headerForTransform = headerTransform(bvc.header.frame, toFrame: finalFrame, container: container)

    bvc.footer.transform = footerForTransform
    bvc.header.transform = headerForTransform
    bvc.readerModeBar?.transform = headerForTransform
}

private func footerTransform( _ frame: CGRect, toFrame finalFrame: CGRect, container: UIView) -> CGAffineTransform {
    let frame = container.convert(frame, to: container)
    let endY = finalFrame.maxY - (frame.size.height / 2)
    let endX = finalFrame.midX
    let translation = CGPoint(x: endX - frame.midX, y: endY - frame.midY)

    let scaleX = finalFrame.width / frame.width

    var transform: CGAffineTransform = .identity
    transform = transform.translatedBy(x: translation.x, y: translation.y)
    transform = transform.scaledBy(x: scaleX, y: scaleX)
    return transform
}

private func headerTransform(_ frame: CGRect, toFrame finalFrame: CGRect, container: UIView) -> CGAffineTransform {
    let frame = container.convert(frame, to: container)
    let endY = finalFrame.minY + (frame.size.height / 2)
    let endX = finalFrame.midX
    let translation = CGPoint(x: endX - frame.midX, y: endY - frame.midY)

    let scaleX = finalFrame.width / frame.width

    var transform: CGAffineTransform = .identity
    transform = transform.translatedBy(x: translation.x, y: translation.y)
    transform = transform.scaledBy(x: scaleX, y: scaleX)
    return transform
}

//MARK: Private Helper Methods
private func calculateCollapsedCellFrameUsingCollectionView(_ collectionView: UICollectionView, atIndex index: Int) -> CGRect {
    if let attr = collectionView.collectionViewLayout.layoutAttributesForItem(at: IndexPath(item: index, section: 0)) {
        return collectionView.convert(attr.frame, to: collectionView.superview)
    } else {
        return .zero
    }
}

private func calculateExpandedCellFrameFromBVC(_ bvc: BrowserViewController) -> CGRect {
    var frame = bvc.webViewContainer.frame

    // If we're navigating to a home panel and we were expecting to show the toolbar, add more height to end frame since
    // there is no toolbar for home panels
    if !bvc.shouldShowFooterForTraitCollection(bvc.traitCollection) {
        return frame
    } else if let url = bvc.tabManager.selectedTab?.url, url.isAboutURL && bvc.toolbar == nil {
        frame.size.height += UIConstants.bottomToolbarHeight
    }

    return frame
}

private func shouldDisplayFooterForBVC(_ bvc: BrowserViewController) -> Bool {
    if bvc.shouldShowFooterForTraitCollection(bvc.traitCollection) {
        if let url = bvc.tabManager.selectedTab?.url {
            return !url.isAboutURL
        }
    }
    return false
}

private func toggleWebViewVisibility(_ show: Bool, usingTabManager tabManager: TabManager) {
    for i in 0..<tabManager.count {
        if let tab = tabManager[i] {
            tab.webView?.isHidden = !show
        }
    }
}

private func resetTransformsForViews(_ views: [UIView?]) {
    for view in views {
        // Reset back to origin
        view?.transform = .identity
    }
}

private func transformToolbarsToFrame(_ toolbars: [UIView?], toRect endRect: CGRect) {
    for toolbar in toolbars {
        // Reset back to origin
        toolbar?.transform = .identity

        // Transform from origin to where we want them to end up
        if let toolbarFrame = toolbar?.frame {
            toolbar?.transform = CGAffineTransformMakeRectToRect(toolbarFrame, toFrame: endRect)
        }
    }
}

private func createTransitionCellFromTab(_ tab: Tab?, withFrame frame: CGRect) -> TabCell {
    let cell = TabCell(frame: frame)
    cell.screenshotView.image = tab?.screenshot
    cell.titleText.text = tab?.displayTitle

    if let favIcon = tab?.displayFavicon {
        cell.favicon.sd_setImage(with: URL(string: favIcon.url)!)
    } else {
        cell.favicon.image = #imageLiteral(resourceName: "defaultFavicon")
    }
    cell.applyTheme(Theme.of(tab))

    return cell
}
