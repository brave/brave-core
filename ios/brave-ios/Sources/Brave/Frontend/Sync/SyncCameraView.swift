// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import BraveShared
import BraveUI
import Data
import Shared
import UIKit

class SyncCameraView: UIView, AVCaptureMetadataOutputObjectsDelegate {
  var captureSession: AVCaptureSession?
  var videoPreviewLayer: AVCaptureVideoPreviewLayer?
  var cameraOverlayView: UIImageView!
  private lazy var cameraAccessButton: RoundInterfaceButton = {
    let button = self.createCameraButton()
    button.setTitle(Strings.Sync.grantCameraAccess, for: .normal)
    button.tintColor = .white
    button.addTarget(self, action: #selector(cameraAccess), for: .touchUpInside)
    return button
  }()

  private lazy var openSettingsButton: RoundInterfaceButton = {
    let button = self.createCameraButton()
    button.setTitle(Strings.openPhoneSettingsActionTitle, for: .normal)
    button.addTarget(self, action: #selector(openSettings), for: .touchUpInside)
    return button
  }()

  var scanCallback: ((_ data: String) -> Void)?

  override init(frame: CGRect) {
    super.init(frame: frame)

    cameraOverlayView = UIImageView(
      image: UIImage(named: "camera-overlay", in: .module, compatibleWith: nil)
    )
    cameraOverlayView.contentMode = .center
    addSubview(cameraOverlayView)
    addSubview(cameraAccessButton)
    addSubview(openSettingsButton)

    [cameraAccessButton, openSettingsButton].forEach { button in
      button.snp.makeConstraints { make in
        make.centerX.equalTo(cameraOverlayView)
        make.centerY.equalTo(cameraOverlayView)
        make.width.equalTo(150)
        make.height.equalTo(40)
      }
    }

    switch AVCaptureDevice.authorizationStatus(for: AVMediaType.video) {
    case .authorized:
      cameraAccessButton.isHidden = true
      openSettingsButton.isHidden = true
      startCapture()
    case .denied:
      cameraAccessButton.isHidden = true
      openSettingsButton.isHidden = false
    default:
      cameraAccessButton.isHidden = false
      openSettingsButton.isHidden = true
    }
  }

  fileprivate func createCameraButton() -> RoundInterfaceButton {
    let button = RoundInterfaceButton(type: .roundedRect)
    button.titleLabel?.font = UIFont.systemFont(ofSize: 14, weight: UIFont.Weight.bold)
    button.setTitleColor(UIColor.white, for: .normal)
    button.backgroundColor = .clear
    button.layer.borderColor = UIColor.white.withAlphaComponent(0.4).cgColor
    button.layer.borderWidth = 1.5

    return button
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func layoutSubviews() {
    if let vpl = videoPreviewLayer {
      vpl.frame = bounds
    }
    cameraOverlayView.frame = bounds
  }

  @objc func cameraAccess() {
    startCapture()
  }

  @objc func openSettings() {
    UIApplication.shared.open(URL(string: UIApplication.openSettingsURLString)!)
  }

  private func startCapture() {
    guard let captureDevice = AVCaptureDevice.default(for: AVMediaType.video) else {
      print("Capture device is nil")
      return
    }

    let input: AVCaptureDeviceInput?
    do {
      input = try AVCaptureDeviceInput(device: captureDevice) as AVCaptureDeviceInput
    } catch let error as NSError {
      debugPrint(error)
      return
    }

    captureSession = AVCaptureSession()

    guard let captureSession = captureSession else {
      print("Capture session is nil")
      return
    }

    captureSession.addInput(input! as AVCaptureInput)

    let captureMetadataOutput = AVCaptureMetadataOutput()
    captureSession.addOutput(captureMetadataOutput)

    captureMetadataOutput.setMetadataObjectsDelegate(self, queue: DispatchQueue.main)
    captureMetadataOutput.metadataObjectTypes = [AVMetadataObject.ObjectType.qr]

    videoPreviewLayer = AVCaptureVideoPreviewLayer(session: captureSession)
    videoPreviewLayer?.videoGravity = AVLayerVideoGravity.resizeAspectFill
    videoPreviewLayer?.frame = layer.bounds
    layer.addSublayer(videoPreviewLayer!)

    startRunning()
    bringSubviewToFront(cameraOverlayView)

    AVCaptureDevice.requestAccess(
      for: AVMediaType.video,
      completionHandler: { (granted: Bool) -> Void in
        DispatchQueue.main.async {
          self.cameraAccessButton.isHidden = true
          if granted {
            self.openSettingsButton.isHidden = true
          } else {
            self.openSettingsButton.isHidden = false
            self.bringSubviewToFront(self.openSettingsButton)
          }
        }
      }
    )
  }

  func startRunning() {
    DispatchQueue.global(qos: .default).async { [weak self] in
      guard let self else { return }

      if self.captureSession?.isRunning == false {
        self.captureSession?.startRunning()
      }
    }
  }

  func stopRunning() {
    if captureSession?.isRunning == true {
      captureSession?.stopRunning()
    }
  }

  func metadataOutput(
    _ output: AVCaptureMetadataOutput,
    didOutput metadataObjects: [AVMetadataObject],
    from connection: AVCaptureConnection
  ) {

    if metadataObjects.isEmpty { return }

    guard let metadataObj = metadataObjects[0] as? AVMetadataMachineReadableCodeObject else {
      assertionFailure("Could not cast metadataObj to AVMetadataMachineReadableCodeObject")
      return
    }
    if metadataObj.type == AVMetadataObject.ObjectType.qr {
      if let callback = scanCallback {
        if let stringValue = metadataObj.stringValue {
          callback(stringValue)
        }
      }
    }
  }

  func cameraOverlayError() {
    NSObject.cancelPreviousPerformRequests(withTarget: self)

    cameraOverlayView.tintColor = .red
    perform(#selector(cameraOverlayNormal), with: self, afterDelay: 1.0)
  }

  func cameraOverlaySuccess() {
    NSObject.cancelPreviousPerformRequests(withTarget: self)

    cameraOverlayView.tintColor = .green
    perform(#selector(cameraOverlayNormal), with: self, afterDelay: 1.0)
  }

  @objc func cameraOverlayNormal() {
    cameraOverlayView.tintColor = .white
  }
}
