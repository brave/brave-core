// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import pop
import Lottie
import BraveRewards

private let log = Logger.browserLogger

protocol Onboardable: class {
    /// Show next on boarding screen if possible.
    /// If last screen is currently presenting, the view is dimissed instead(onboarding finished).
    func presentNextScreen(current: OnboardingViewController)
    /// Show previous on boarding screen if possible.
    func presentPreviousScreen(current: OnboardingViewController)
    /// Skip all onboarding screens, onboarding is considered as completed.
    func skip()
}

protocol OnboardingControllerDelegate: class {
    func onboardingCompleted(_ onboardingController: OnboardingNavigationController)
    func onboardingSkipped(_ onboardingController: OnboardingNavigationController)
}

enum OnboardingViewAnimationID: Int {
    case background = 1
    case details
    case detailsContent
}

class OnboardingNavigationController: UINavigationController {
    
    private struct UX {
        /// The onboarding screens are showing as a modal on iPads.
        static let preferredModalSize = CGSize(width: 375, height: 667)
    }
    
    weak var onboardingDelegate: OnboardingControllerDelegate?
    
    fileprivate enum Screens {
        case privacyConsent
        case searchEnginePicker
        case shieldsInfo
        
        /// Returns new ViewController associated with the screen type
        func viewController(with profile: Profile, rewards: BraveRewards?, theme: Theme) -> OnboardingViewController {
            switch self {
            case .privacyConsent: return OnboardingPrivacyConsentViewController(profile: profile, rewards: rewards, theme: theme)
            case .searchEnginePicker:
                return OnboardingSearchEnginesViewController(profile: profile, rewards: rewards, theme: theme)
            case .shieldsInfo:
                return OnboardingShieldsViewController(profile: profile, rewards: rewards, theme: theme)
            }
        }
        
        var type: OnboardingViewController.Type {
            switch self {
            case .privacyConsent: return OnboardingPrivacyConsentViewController.self
            case .searchEnginePicker: return OnboardingSearchEnginesViewController.self
            case .shieldsInfo: return OnboardingShieldsViewController.self
            }
        }
    }
    
    private static var screens: [Screens] = {
        var introScreens: [Screens] = [.searchEnginePicker, .shieldsInfo]
        if Preferences.URP.referralCode.value == nil && UIPasteboard.general.hasStrings {
            introScreens.insert(.privacyConsent, at: 0)
        }
        return introScreens
    }()
    
    convenience init?(profile: Profile, rewards: BraveRewards?, theme: Theme) {
        guard let firstViewController = Self.screens.first?.viewController(with: profile, rewards: rewards, theme: theme) else {
            return nil
        }
        self.init(rootViewController: firstViewController)
        firstViewController.delegate = self
        
        isNavigationBarHidden = true
        self.delegate = self

        modalPresentationStyle = UIDevice.current.userInterfaceIdiom == .phone ? .fullScreen : .formSheet
        
        if #available(iOS 13.0, *) {
            // Prevent dismissing the modal by swipe
            isModalInPresentation = true
        }
        preferredContentSize = UX.preferredModalSize
        
        let backgroundView = UIView().then {
            $0.backgroundColor = #colorLiteral(red: 0.1176470588, green: 0.1254901961, blue: 0.1607843137, alpha: 1)
        }
        
        view.insertSubview(backgroundView, at: 0)
        backgroundView.snp.makeConstraints {
            $0.edges.equalToSuperview()
        }
    }
}

extension OnboardingNavigationController: Onboardable {
    
    func presentNextScreen(current: OnboardingViewController) {
        let index = Self.screens.firstIndex { $0.type == type(of: current) }
        
        guard let nextIndex = index?.advanced(by: 1),
            let nextScreen = Self.screens[safe: nextIndex]?.viewController(with: current.profile, rewards: current.rewards, theme: current.theme) else {
                log.info("Last screen reached, onboarding is complete")
                onboardingDelegate?.onboardingCompleted(self)
                return
        }
        
        nextScreen.delegate = self
        
        pushViewController(nextScreen, animated: true)
    }
    
    func presentPreviousScreen(current: OnboardingViewController) {
        let index = Self.screens.firstIndex { $0.type == type(of: current) }
        
        guard let previousIndex = index?.advanced(by: -1), let previousScreen = viewControllers[previousIndex] as? OnboardingViewController else {
                log.info("First screen reached")
                return
        }
        previousScreen.delegate = self
        
        popToViewController(previousScreen, animated: true)
    }
    
    func skip() {
        onboardingDelegate?.onboardingSkipped(self)
    }
}

extension OnboardingNavigationController: UINavigationControllerDelegate {
    func navigationController(_ navigationController: UINavigationController, animationControllerFor operation: UINavigationController.Operation, from fromVC: UIViewController, to toVC: UIViewController) -> UIViewControllerAnimatedTransitioning? {

         switch operation {
         case .push:
            return CustomAnimator(isPresenting: true, shouldFadeGraphics: false)
         default:
             return CustomAnimator(isPresenting: false, shouldFadeGraphics: false)
         }
    }
}

// Disabling orientation changes
extension OnboardingNavigationController {
    override var preferredStatusBarStyle: UIStatusBarStyle {
        return .default
    }
    
    override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
        return .portrait
    }
    
    override var preferredInterfaceOrientationForPresentation: UIInterfaceOrientation {
        return .portrait
    }
    
    override var shouldAutorotate: Bool {
        return false
    }
}

class CustomAnimator: NSObject, UIViewControllerAnimatedTransitioning {
    
    let isPresenting: Bool
    let shouldFadeGraphics: Bool
    
    init(isPresenting: Bool, shouldFadeGraphics: Bool) {
        self.isPresenting = isPresenting
        self.shouldFadeGraphics = shouldFadeGraphics
    }
    
    func animateTransition(using transitionContext: UIViewControllerContextTransitioning) {
        let container = transitionContext.containerView
        
        guard let fromView = transitionContext.view(forKey: UITransitionContextViewKey.from) else { return }
        
        guard let toView = transitionContext.view(forKey: UITransitionContextViewKey.to) else { return }
        
        //Setup
        fromView.frame = container.bounds
        toView.frame = container.bounds
        container.addSubview(toView)
        fromView.layoutIfNeeded()
        toView.layoutIfNeeded()
        
        //Get animatable views
        let fBackground = fromView.subview(with: OnboardingViewAnimationID.background.rawValue)
        let fDetails = fromView.subview(with: OnboardingViewAnimationID.details.rawValue)
        let fDetailsContent = fromView.subview(with: OnboardingViewAnimationID.detailsContent.rawValue)
        
        let tBackground = toView.subview(with: OnboardingViewAnimationID.background.rawValue)
        let tDetails = toView.subview(with: OnboardingViewAnimationID.details.rawValue)
        let tDetailsContent = toView.subview(with: OnboardingViewAnimationID.detailsContent.rawValue)

        //Setup animation
        fBackground?.alpha = 1.0
        fDetails?.alpha = 1.0
        fDetails?.layer.maskedCorners = [.layerMinXMinYCorner, .layerMaxXMinYCorner]
        fDetailsContent?.alpha = 1.0
        
        tBackground?.alpha = 0.0
        tDetails?.alpha = 0.0
        tDetailsContent?.alpha = 0.0
        
        let inset: CGFloat = 0.0
        var fDetailsFrame = (fDetails?.bounds ?? .zero)
        fDetailsFrame.origin.y = (container.frame.height - container.frame.origin.y) - fDetailsFrame.height
        fDetailsFrame = fDetailsFrame.offsetBy(dx: 0.0, dy: -inset)
        
        var tDetailsFrame = (tDetails?.bounds ?? .zero)
        tDetailsFrame.origin.y = (container.frame.height - container.frame.origin.y) - tDetailsFrame.height
        tDetailsFrame = tDetailsFrame.offsetBy(dx: 0.0, dy: -inset)
        
        //Pause animations..
        if !shouldFadeGraphics, let fAnimation = fBackground as? AnimationView, let tAnimation = tBackground as? AnimationView {
            
            fAnimation.pause()
            tAnimation.play(fromProgress: fAnimation.currentProgress, toProgress: 1.0)
            //tAnimation.pause()
            fAnimation.stop()
        }

        //fade contents of white panel
        POPBasicAnimation(propertyNamed: kPOPLayerOpacity)?.do {
            $0.toValue = 0.0
            $0.duration = 0.2
            $0.beginTime = CACurrentMediaTime()
            fDetailsContent?.layer.pop_add($0, forKey: "alpha")
            
            $0.completionBlock = { _, _ in
                tDetails?.alpha = 1.0
            }
        }
        
        POPBasicAnimation(propertyNamed: kPOPLayerOpacity)?.do {
            $0.toValue = shouldFadeGraphics ? 0.0 : 1.0
            $0.duration = 0.2
            fBackground?.layer.pop_add($0, forKey: "alpha")
            $0.completionBlock = { _, _ in
                if !self.shouldFadeGraphics, let tAnimation = tBackground as? AnimationView {
                    tAnimation.play()
                }
            }
        }
        
        //resize white background to size on next screen
        POPBasicAnimation(propertyNamed: kPOPViewFrame)?.do {
            $0.fromValue = fDetailsFrame
            $0.toValue = tDetailsFrame
            $0.duration = 0.3
            $0.beginTime = CACurrentMediaTime() + 0.1
            fDetails?.layer.pop_add($0, forKey: "frame")
        }
        
        POPBasicAnimation(propertyNamed: kPOPLayerCornerRadius)?.do {
            $0.toValue = 12.0
            $0.duration = 0.3
            $0.beginTime = CACurrentMediaTime() + 0.1
            fDetails?.layer.pop_add($0, forKey: "cornerRadius")
        }
        
        //fade in background of next screen and its contents..
        POPBasicAnimation(propertyNamed: kPOPLayerOpacity)?.do {
            $0.toValue = 1.0
            $0.duration = 0.4
            $0.beginTime = CACurrentMediaTime() + 0.3
            tBackground?.layer.pop_add($0, forKey: "alpha")
            
            if !shouldFadeGraphics {
                $0.completionBlock = { _, _ in
                    fBackground?.layer.opacity = 0.0
                }
            }
        }
        
        POPBasicAnimation(propertyNamed: kPOPLayerOpacity)?.do {
            $0.toValue = 1.0
            $0.duration = 0.4
            $0.beginTime = CACurrentMediaTime() + 0.3
            tDetailsContent?.layer.pop_add($0, forKey: "alpha")
        }
        
        DispatchQueue.main.asyncAfter(deadline: .now() + self.transitionDuration(using: transitionContext)) {
            transitionContext.completeTransition(!transitionContext.transitionWasCancelled)
        }
    }
    
    func transitionDuration(using transitionContext: UIViewControllerContextTransitioning?) -> TimeInterval {
        return 0.7
    }
}

private extension UIView {
    func subview(with tag: Int) -> UIView? {
        if self.tag == tag {
            return self
        }
        
        for view in self.subviews {
            if view.tag == tag {
                return view
            }
            
            if let view = view.subview(with: tag) {
                return view
            }
        }
        return nil
    }
}
