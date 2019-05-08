/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import AVFoundation
import BraveShared
import Data

class SyncPairCameraViewController: SyncViewController {
    
    var syncHandler: (([Int]?) -> Void)?
    var cameraView: SyncCameraView!
    var titleLabel: UILabel!
    var descriptionLabel: UILabel!
    var enterWordsButton: RoundInterfaceButton!
    
    var loadingView: UIView!
    let loadingSpinner = UIActivityIndicatorView(style: .whiteLarge)
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = Strings.ScanSyncCode

        let stackView = UIStackView()
        stackView.axis = .vertical
        stackView.distribution = .equalSpacing
        stackView.alignment = .center
        stackView.spacing = 4
        view.addSubview(stackView)

        stackView.snp.makeConstraints { make in
            make.top.equalTo(self.view.safeArea.top).offset(16)
            make.left.right.equalTo(self.view).inset(16)
            make.bottom.equalTo(self.view.safeArea.bottom).inset(16)
        }
        
        cameraView = SyncCameraView()
        cameraView.translatesAutoresizingMaskIntoConstraints = false
        cameraView.backgroundColor = UIColor.black
        cameraView.layer.cornerRadius = 4
        cameraView.layer.masksToBounds = true
        cameraView.scanCallback = { data in
            
            if !DeviceInfo.hasConnectivity() {
                self.present(SyncAlerts.noConnection, animated: true)
                return
            }
            
            // TODO: Check data against sync api

            // TODO: Functional, but needs some cleanup
            struct Scanner { static var Lock = false }
            if let bytes = SyncCrypto().splitBytes(fromJoinedBytes: data) {
                if Scanner.Lock {
                    // Have internal, so camera error does not show
                    return
                }
                
                Scanner.Lock = true
                self.cameraView.cameraOverlaySucess()
                // Freezing the camera frame after QR has been scanned.
                self.cameraView.captureSession?.stopRunning()
                
                // Vibrate.
                AudioServicesPlayAlertSound(SystemSoundID(kSystemSoundID_Vibrate))  

                // Forced timeout
                DispatchQueue.main.asyncAfter(deadline: DispatchTime.now() + Double(Int64(25.0) * Int64(NSEC_PER_SEC)) / Double(NSEC_PER_SEC), execute: {
                    Scanner.Lock = false
                    self.cameraView.cameraOverlayError()
                })
                
                // If multiple calls get in here due to race conditions it isn't a big deal
                
                self.syncHandler?(bytes)

            } else {
                self.cameraView.cameraOverlayError()
            }
        }

        stackView.addArrangedSubview(cameraView)

        let titleDescriptionStackView = UIStackView()
        titleDescriptionStackView.axis = .vertical
        titleDescriptionStackView.spacing = 4
        titleDescriptionStackView.alignment = .center
        titleDescriptionStackView.setContentCompressionResistancePriority(UILayoutPriority(rawValue: 250), for: .vertical)
        
        titleLabel = UILabel()
        titleLabel.translatesAutoresizingMaskIntoConstraints = false
        titleLabel.font = UIFont.systemFont(ofSize: 20, weight: UIFont.Weight.semibold)
        titleLabel.textColor = BraveUX.GreyJ
        titleLabel.text = Strings.SyncToDevice
        titleDescriptionStackView.addArrangedSubview(titleLabel)

        descriptionLabel = UILabel()
        descriptionLabel.translatesAutoresizingMaskIntoConstraints = false
        descriptionLabel.font = UIFont.systemFont(ofSize: 15, weight: UIFont.Weight.regular)
        descriptionLabel.textColor = BraveUX.GreyH
        descriptionLabel.numberOfLines = 0
        descriptionLabel.lineBreakMode = .byWordWrapping
        descriptionLabel.textAlignment = .center
        descriptionLabel.text = Strings.SyncToDeviceDescription
        titleDescriptionStackView.addArrangedSubview(descriptionLabel)

        let textStackView = UIStackView(arrangedSubviews: [UIView.spacer(.horizontal, amount: 16),
                                                           titleDescriptionStackView,
                                                           UIView.spacer(.horizontal, amount: 16)])

        stackView.addArrangedSubview(textStackView)

        enterWordsButton = RoundInterfaceButton(type: .roundedRect)
        enterWordsButton.translatesAutoresizingMaskIntoConstraints = false
        enterWordsButton.setTitle(Strings.EnterCodeWords, for: .normal)
        enterWordsButton.titleLabel?.font = UIFont.systemFont(ofSize: 15, weight: UIFont.Weight.semibold)
        enterWordsButton.setTitleColor(BraveUX.GreyH, for: .normal)
        enterWordsButton.addTarget(self, action: #selector(SEL_enterWords), for: .touchUpInside)
        stackView.addArrangedSubview(enterWordsButton)
        
        loadingSpinner.startAnimating()
        
        loadingView = UIView()
        loadingView.translatesAutoresizingMaskIntoConstraints = false
        loadingView.backgroundColor = UIColor(white: 0.5, alpha: 0.5)
        loadingView.isHidden = true
        loadingView.addSubview(loadingSpinner)
        cameraView.addSubview(loadingView)
        
        edgesForExtendedLayout = UIRectEdge()

        cameraView.snp.makeConstraints { (make) in
            if UIDevice.current.userInterfaceIdiom == .pad {
                make.size.equalTo(400)
            } else {
                make.size.equalTo(self.view.snp.width).multipliedBy(0.9)
            }
        }

        loadingView.snp.makeConstraints { make in
            make.left.right.top.bottom.equalTo(cameraView)
        }
        
        loadingSpinner.snp.makeConstraints { make in
            make.center.equalTo(loadingSpinner.superview!)
        }
    }

    override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
        coordinator.animate(alongsideTransition: nil) { _ in
            self.cameraView.videoPreviewLayer?.connection?.videoOrientation = AVCaptureVideoOrientation(ui: UIApplication.shared.statusBarOrientation)
        }
    }
    
    @objc func SEL_enterWords() {
        let wordsVC = SyncPairWordsViewController()
        wordsVC.syncHandler = self.syncHandler
        navigationController?.pushViewController(wordsVC, animated: true)
    }
}

extension SyncPairCameraViewController: NavigationPrevention {
    func enableNavigationPrevention() {
        loadingView.isHidden = false
        navigationItem.hidesBackButton = true
        enterWordsButton.isEnabled = false
    }

    func disableNavigationPrevention() {
        loadingView.isHidden = true
        navigationItem.hidesBackButton = false
        enterWordsButton.isEnabled = true
    }
}

extension AVCaptureVideoOrientation {
    var uiInterfaceOrientation: UIInterfaceOrientation {
        get {
            switch self {
            case .landscapeLeft:        return .landscapeLeft
            case .landscapeRight:       return .landscapeRight
            case .portrait:             return .portrait
            case .portraitUpsideDown:   return .portraitUpsideDown
            }
        }
    }

    init(ui: UIInterfaceOrientation) {
        switch ui {
        case .landscapeRight:       self = .landscapeRight
        case .landscapeLeft:        self = .landscapeLeft
        case .portrait:             self = .portrait
        case .portraitUpsideDown:   self = .portraitUpsideDown
        default:                    self = .portrait
        }
    }
}
