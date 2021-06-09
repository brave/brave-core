/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import AVFoundation
import SnapKit
import Shared

private let log = Logger.browserLogger

private struct QRCodeViewControllerUX {
    static let maskViewBackgroundColor = UIColor(red: 0, green: 0, blue: 0, alpha: 0.5)
    static let isLightingNavigationItemColor = UIColor(red: 0.45, green: 0.67, blue: 0.84, alpha: 1)
}

protocol QRCodeViewControllerDelegate {
    func didScanQRCodeWithURL(_ url: URL)
    func didScanQRCodeWithText(_ text: String)
}

class QRCodeViewController: UIViewController {
    var qrCodeDelegate: QRCodeViewControllerDelegate?
    
    public static var hasCameraSupport: Bool {
        !AVCaptureDevice.DiscoverySession(deviceTypes: [.builtInWideAngleCamera], mediaType: .video, position: .back).devices.isEmpty
    }
    
    public static var hasCameraPermissions: Bool {
        // Status Restricted - Hardware Restriction such as parental controls
        let status = AVCaptureDevice.authorizationStatus(for: AVMediaType.video)
        return status != .denied && status != .restricted
    }

    fileprivate lazy var captureSession: AVCaptureSession = {
        let session = AVCaptureSession()
        session.sessionPreset = AVCaptureSession.Preset.high
        return session
    }()

    private lazy var captureDevice: AVCaptureDevice? = {
        return AVCaptureDevice.default(for: AVMediaType.video)
    }()

    private var videoPreviewLayer: AVCaptureVideoPreviewLayer?
    private let scanLine: UIImageView = UIImageView(image: #imageLiteral(resourceName: "qrcode-scanLine"))
    private let scanBorder: UIImageView = UIImageView(image: #imageLiteral(resourceName: "qrcode-scanBorder"))
    private lazy var instructionsLabel: UILabel = {
        let label = UILabel()
        label.text = Strings.scanQRCodeInstructionsLabel
        label.textColor = .white
        label.textAlignment = .center
        label.numberOfLines = 0
        return label
    }()
    private var maskView: UIView = UIView()
    private var isAnimationing: Bool = false
    private var isLightOn: Bool = false
    private var shapeLayer: CAShapeLayer = CAShapeLayer()

    private var scanRange: CGRect {
        let size = UIDevice.current.userInterfaceIdiom == .pad ?
            CGSize(width: view.frame.width / 2, height: view.frame.width / 2) :
            CGSize(width: view.frame.width / 3 * 2, height: view.frame.width / 3 * 2)
        var rect = CGRect(size: size)
        rect.center = self.view.bounds.center
        return rect
    }

    private var scanBorderHeight: CGFloat {
        return UIDevice.current.userInterfaceIdiom == .pad ?
            view.frame.width / 2 : view.frame.width / 3 * 2
    }

    override func viewDidLoad() {
        super.viewDidLoad()

        title = Strings.scanQRCodeViewTitle
        setupNavigationBar()

        let getAuthorizationStatus = AVCaptureDevice.authorizationStatus(for: AVMediaType.video)
        if getAuthorizationStatus != .denied {
            setupCamera()
        } else {
            let alert = UIAlertController(title: "", message: Strings.scanQRCodePermissionErrorMessage, preferredStyle: .alert)
            alert.addAction(UIAlertAction(title: Strings.scanQRCodeErrorOKButton, style: .default, handler: nil))
            self.present(alert, animated: true, completion: nil)
        }

        maskView.backgroundColor = QRCodeViewControllerUX.maskViewBackgroundColor
        view.addSubview(maskView)
        view.addSubview(scanBorder)
        view.addSubview(scanLine)
        view.addSubview(instructionsLabel)

        setupConstraints()
        let rectPath = UIBezierPath(rect: view.bounds)
        rectPath.append(UIBezierPath(rect: scanRange).reversing())
        shapeLayer.path = rectPath.cgPath
        maskView.layer.mask = shapeLayer

        isAnimationing = true
        startScanLineAnimation()
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        captureSession.startRunning()
        
        if !isAnimationing {
            isAnimationing = true
            startScanLineAnimation()
        }
    }
    
    override func viewDidAppear(_ animated: Bool) {
        super.viewDidAppear(animated)
        
        shapeLayer.removeFromSuperlayer()
        let rectPath = UIBezierPath(rect: self.view.bounds)
        rectPath.append(UIBezierPath(rect: scanRange).reversing())
        shapeLayer.path = rectPath.cgPath
        maskView.layer.mask = shapeLayer
    }

    override func viewWillDisappear(_ animated: Bool) {
        super.viewWillDisappear(animated)
        
        captureSession.stopRunning()
        stopScanLineAnimation()
    }

    private func setupConstraints() {
        maskView.snp.makeConstraints { (make) in
            make.edges.equalTo(self.view)
        }
        
        if UIDevice.current.userInterfaceIdiom == .pad {
            scanBorder.snp.makeConstraints { (make) in
                make.center.equalTo(self.view)
                make.width.height.equalTo(view.frame.width / 2)
            }
        } else {
            scanBorder.snp.makeConstraints { (make) in
                make.center.equalTo(self.view)
                make.width.height.equalTo(view.frame.width / 3 * 2)
            }
        }
        
        scanLine.snp.makeConstraints { (make) in
            make.left.equalTo(scanBorder.snp.left)
            make.top.equalTo(scanBorder.snp.top).offset(6)
            make.width.equalTo(scanBorder.snp.width)
            make.height.equalTo(6)
        }

        instructionsLabel.snp.makeConstraints { (make) in
            make.left.right.equalTo(self.view.safeAreaLayoutGuide)
            make.top.equalTo(scanBorder.snp.bottom).offset(15)
        }
    }
    
    private func setupNavigationBar() {
        guard let captureDevice = self.captureDevice else {
            dismiss(animated: false)
            return
        }
        
        // Setup the NavigationItem
        navigationItem.rightBarButtonItem = UIBarButtonItem(image: #imageLiteral(resourceName: "playlist_exit").template, style: .plain, target: self, action: #selector(goBack))
        navigationItem.rightBarButtonItem?.tintColor = .bravePrimary

        navigationItem.leftBarButtonItem = UIBarButtonItem(image: #imageLiteral(resourceName: "qrcode-light").template, style: .plain, target: self, action: #selector(openLight))
        if captureDevice.hasTorch {
            navigationItem.leftBarButtonItem?.tintColor = .bravePrimary
        } else {
            navigationItem.leftBarButtonItem?.tintColor = .braveDisabled
            navigationItem.leftBarButtonItem?.isEnabled = false
        }
    }

    @objc func startScanLineAnimation() {
        if !isAnimationing {
            return
        }
        
        view.layoutIfNeeded()
        view.setNeedsLayout()
        UIView.animate(withDuration: 2.4, animations: {
            self.scanLine.snp.updateConstraints({ (make) in
                make.top.equalTo(self.scanBorder.snp.top).offset(self.scanBorder.bounds.height - 6)
            })
            self.view.layoutIfNeeded()
        }, completion: { _ in
            self.scanLine.snp.updateConstraints({ (make) in
                make.top.equalTo(self.scanBorder.snp.top).offset(6)
            })
            self.perform(#selector(self.startScanLineAnimation), with: nil, afterDelay: 0)
        })
    }

    func stopScanLineAnimation() {
        isAnimationing = false
    }

    @objc func goBack() {
        dismiss(animated: true, completion: nil)
    }

    @objc func openLight() {
        guard let captureDevice = self.captureDevice else {
            return
        }

        if isLightOn {
            do {
                try captureDevice.lockForConfiguration()
                captureDevice.torchMode = AVCaptureDevice.TorchMode.off
                captureDevice.unlockForConfiguration()
                navigationItem.leftBarButtonItem?.image = #imageLiteral(resourceName: "qrcode-light")
                navigationItem.leftBarButtonItem?.tintColor = .bravePrimary
            } catch {
                log.error(error)
            }
        } else {
            do {
                try captureDevice.lockForConfiguration()
                captureDevice.torchMode = AVCaptureDevice.TorchMode.on
                captureDevice.unlockForConfiguration()
                navigationItem.leftBarButtonItem?.image = #imageLiteral(resourceName: "qrcode-isLighting")
                navigationItem.leftBarButtonItem?.tintColor = .braveOrange
            } catch {
                log.error(error)
            }
        }
        isLightOn = !isLightOn
    }

    func setupCamera() {
        guard let captureDevice = self.captureDevice else {
            dismiss(animated: false)
            return
        }

        do {
            let input = try AVCaptureDeviceInput(device: captureDevice)
            captureSession.addInput(input)
        } catch {
            log.error(error)
        }
        
        let output = AVCaptureMetadataOutput()
        if captureSession.canAddOutput(output) {
            captureSession.addOutput(output)
            output.setMetadataObjectsDelegate(self, queue: DispatchQueue.main)
            output.metadataObjectTypes = [AVMetadataObject.ObjectType.qr]
        }
        
        let videoPreviewLayer = AVCaptureVideoPreviewLayer(session: captureSession)
        videoPreviewLayer.videoGravity = AVLayerVideoGravity.resizeAspectFill
        videoPreviewLayer.frame = view.bounds
        view.layer.addSublayer(videoPreviewLayer)
        self.videoPreviewLayer = videoPreviewLayer
        captureSession.startRunning()

    }

    override func willAnimateRotation(to toInterfaceOrientation: UIInterfaceOrientation, duration: TimeInterval) {
        super.willAnimateRotation(to: toInterfaceOrientation, duration: duration)
        
        shapeLayer.removeFromSuperlayer()
        let rectPath = UIBezierPath(rect: view.bounds)
        rectPath.append(UIBezierPath(rect: scanRange).reversing())
        shapeLayer.path = rectPath.cgPath
        maskView.layer.mask = shapeLayer

        guard let videoPreviewLayer = self.videoPreviewLayer else {
            return
        }
        
        videoPreviewLayer.frame = view.bounds
        switch toInterfaceOrientation {
        case .portrait:
            videoPreviewLayer.connection?.videoOrientation = .portrait
        case .landscapeLeft:
            videoPreviewLayer.connection?.videoOrientation = .landscapeLeft
        case .landscapeRight:
            videoPreviewLayer.connection?.videoOrientation = .landscapeRight
        case .portraitUpsideDown:
            videoPreviewLayer.connection?.videoOrientation = .portraitUpsideDown
        default:
            videoPreviewLayer.connection?.videoOrientation = .portrait
        }
    }
}

extension QRCodeViewController: AVCaptureMetadataOutputObjectsDelegate {
    func metadataOutput(_ output: AVCaptureMetadataOutput, didOutput metadataObjects: [AVMetadataObject], from connection: AVCaptureConnection) {
        if metadataObjects.isEmpty {
            captureSession.stopRunning()
            let alert = AlertController(title: "", message: Strings.scanQRCodeInvalidDataErrorMessage, preferredStyle: .alert)
            alert.addAction(UIAlertAction(title: Strings.scanQRCodeErrorOKButton, style: .default, handler: { [weak self] _ in
                self?.captureSession.startRunning()
            }), accessibilityIdentifier: "qrCodeAlert.okButton")
            present(alert, animated: true, completion: nil)
        } else {
            captureSession.stopRunning()
            stopScanLineAnimation()
            dismiss(animated: true, completion: {
                guard let metaData = metadataObjects.first as? AVMetadataMachineReadableCodeObject, let qrCodeDelegate = self.qrCodeDelegate, let text = metaData.stringValue else {
                    log.debug("Unable to scan QR code")
                        return
                }

                if let url = URIFixup.getURL(text) {
                    qrCodeDelegate.didScanQRCodeWithURL(url)
                } else {
                    qrCodeDelegate.didScanQRCodeWithText(text)
                }
            })
        }
    }
}

class QRCodeNavigationController: UINavigationController {
    override open var preferredStatusBarStyle: UIStatusBarStyle {
        return .lightContent
    }
}
