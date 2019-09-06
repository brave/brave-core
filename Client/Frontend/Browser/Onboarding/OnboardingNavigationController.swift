// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared

private let log = Logger.browserLogger

protocol Onboardable: class {
    /// Show next on boarding screen if possible.
    /// If last screen is currently presenting, the view is dimissed instead(onboarding finished).
    func presentNextScreen(current: OnboardingViewController)
    /// Skip all onboarding screens, onboarding is considered as completed.
    func skip()
}

protocol OnboardingControllerDelegate: class {
    func onboardingCompleted(_ onboardingController: OnboardingNavigationController)
}

class OnboardingNavigationController: UINavigationController {
    
    private struct UX {
        /// The onboarding screens are showing as a modal on iPads.
        static let preferredModalSize = CGSize(width: 375, height: 667)
    }
    
    weak var onboardingDelegate: OnboardingControllerDelegate?
    
    enum OnboardingType {
        case newUser
        case existingUser
        
        /// Returns a list of onboarding screens for given type.
        /// Screens should be sorted in order of which they are presented to the user.
        fileprivate var screens: [Screens] {
            switch self {
            case .newUser: return [.searchEnginePicker, .shieldsInfo, /* .rewardsInfo, .adsInfo */]
            case .existingUser: return [/* .rewardsInfo, .adsInfo */]
            }
        }
    }
    
    fileprivate enum Screens {
        case searchEnginePicker
        case shieldsInfo
        
        /// Returns new ViewController associated with the screen type
        func viewController(with profile: Profile, theme: Theme) -> OnboardingViewController {
            switch self {
            case .searchEnginePicker:
                return OnboardingSearchEnginesViewController(profile: profile, theme: theme)
            case .shieldsInfo:
                return OnboardingShieldsViewController(profile: profile, theme: theme)
            }
        }
        
        var type: OnboardingViewController.Type {
            switch self {
            case .searchEnginePicker: return OnboardingSearchEnginesViewController.self
            case .shieldsInfo: return OnboardingShieldsViewController.self
            }
        }
    }
    
    private(set) var onboardingType: OnboardingType?
    
    convenience init?(profile: Profile, onboardingType: OnboardingType, theme: Theme) {
        guard let firstScreen = onboardingType.screens.first else { return nil }
        
        let firstViewController = firstScreen.viewController(with: profile, theme: theme)
        self.init(rootViewController: firstViewController)
        self.onboardingType = onboardingType
        firstViewController.delegate = self
        
        isNavigationBarHidden = true
        
        modalPresentationStyle = UIDevice.current.userInterfaceIdiom == .phone ? .fullScreen : .formSheet
        
        if #available(iOS 13.0, *) {
            // Prevent dismissing the modal by swipe
            isModalInPresentation = true
        }
        preferredContentSize = UX.preferredModalSize
    }
}

extension OnboardingNavigationController: Onboardable {
    
    func presentNextScreen(current: OnboardingViewController) {
        guard let allScreens = onboardingType?.screens else { return }
        let index = allScreens.firstIndex { $0.type == type(of: current) }
        
        guard let nextIndex = index?.advanced(by: 1),
            let nextScreen = allScreens[safe: nextIndex]?.viewController(with: current.profile, theme: current.theme) else {
                log.info("Last screen reached, onboarding is complete")
                onboardingDelegate?.onboardingCompleted(self)
                return
        }
        
        nextScreen.delegate = self
        
        pushViewController(nextScreen, animated: true)
    }
    
    func skip() {
        onboardingDelegate?.onboardingCompleted(self)
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
