// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import BraveShared
import Foundation
import Shared
import UIKit

class RecentSearchQRCodeScannerController: UIViewController {

  private let scannerView = ScannerView()
  private var didScan: Bool = false
  private var onDidScan: (_ string: String) -> Void

  public static var hasCameraSupport: Bool {
    if ProcessInfo.processInfo.isiOSAppOnVisionOS {
      // Apps on VisionOS can't access the main camera
      return false
    }
    return !AVCaptureDevice.DiscoverySession(
      deviceTypes: [.builtInWideAngleCamera],
      mediaType: .video,
      position: .back
    ).devices.isEmpty
  }

  public static var hasCameraPermissions: Bool {
    // Status Restricted - Hardware Restriction such as parental controls
    let status = AVCaptureDevice.authorizationStatus(for: AVMediaType.video)
    return status != .denied && status != .restricted
  }

  init(onDidScan: @escaping (_ string: String) -> Void) {
    self.onDidScan = onDidScan
    super.init(nibName: nil, bundle: nil)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    title = Strings.recentSearchScannerTitle
    navigationItem.rightBarButtonItem = UIBarButtonItem(
      barButtonSystemItem: .done,
      target: self,
      action: #selector(tappedDone)
    )

    view.addSubview(scannerView)
    scannerView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    scannerView.cameraView.scanCallback = { [weak self] string in
      guard let self = self, !string.isEmpty, !self.didScan else { return }
      // Feedback indicating code scan is finalized
      AudioServicesPlayAlertSound(SystemSoundID(kSystemSoundID_Vibrate))
      UIImpactFeedbackGenerator(style: .medium).vibrate()

      self.didScan = true
      self.onDidScan(string)
      self.dismiss(animated: true, completion: nil)
    }
  }

  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)

    scannerView.cameraView.stopRunning()
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)

    scannerView.cameraView.stopRunning()
  }

  override func viewDidAppear(_ animated: Bool) {
    if let orientation = view.window?.windowScene?.interfaceOrientation {
      scannerView.cameraView.videoPreviewLayer?.connection?.videoOrientation =
        AVCaptureVideoOrientation(ui: orientation)
    }
    scannerView.cameraView.startRunning()
  }

  override func viewWillTransition(
    to size: CGSize,
    with coordinator: UIViewControllerTransitionCoordinator
  ) {
    scannerView.cameraView.stopRunning()

    coordinator.animate(alongsideTransition: nil) { [weak self] _ in
      guard let self else { return }

      if let orientation = self.view.window?.windowScene?.interfaceOrientation {
        self.scannerView.cameraView.videoPreviewLayer?.connection?.videoOrientation =
          AVCaptureVideoOrientation(ui: orientation)
      }
      self.scannerView.cameraView.startRunning()
    }
  }

  // MARK: - Actions

  @objc private func tappedDone() {
    dismiss(animated: true)
  }
}

extension RecentSearchQRCodeScannerController {
  class ScannerView: UIView {
    let cameraView = SyncCameraView().then {
      $0.backgroundColor = .black
      $0.layer.cornerRadius = 4
      $0.layer.cornerCurve = .continuous
    }

    private let scrollView = UIScrollView()
    private let stackView = UIStackView().then {
      $0.axis = .vertical
      $0.spacing = 6
      $0.alignment = .leading
    }

    private let titleLabel = UILabel().then {
      $0.text = Strings.recentSearchScannerDescriptionTitle
      $0.font = .systemFont(ofSize: 17, weight: .semibold)
      $0.numberOfLines = 0
      $0.textColor = .braveLabel
    }

    private let bodyLabel = UILabel().then {
      $0.text = Strings.recentSearchScannerDescriptionBody
      $0.font = .systemFont(ofSize: 17)
      $0.numberOfLines = 0
      $0.textColor = .braveLabel
    }

    override init(frame: CGRect) {
      super.init(frame: frame)

      backgroundColor = .secondaryBraveBackground

      addSubview(cameraView)
      addSubview(scrollView)
      scrollView.addSubview(stackView)
      stackView.addStackViewItems(
        .view(titleLabel),
        .view(bodyLabel)
      )

      cameraView.snp.makeConstraints {
        $0.top.equalTo(self.safeAreaLayoutGuide).inset(10)
        $0.leading.greaterThanOrEqualTo(self.safeAreaLayoutGuide).inset(10)
        $0.trailing.lessThanOrEqualTo(self.safeAreaLayoutGuide).inset(10)
        $0.centerX.equalToSuperview()
        $0.height.equalTo(cameraView.snp.width)
        $0.width.lessThanOrEqualTo(375)
      }

      scrollView.snp.makeConstraints {
        $0.top.equalTo(cameraView.snp.bottom).offset(10)
        $0.leading.trailing.bottom.equalToSuperview()
      }
      scrollView.contentLayoutGuide.snp.makeConstraints {
        $0.top.bottom.equalTo(stackView)
        $0.width.equalToSuperview()
      }
      stackView.snp.makeConstraints {
        $0.edges.equalToSuperview().inset(10)
      }
    }

    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
  }
}
