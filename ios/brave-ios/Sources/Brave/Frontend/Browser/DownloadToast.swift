// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Shared
import SnapKit
import UIKit

struct DownloadToastUX {
  static let toastBackgroundColor = UIColor.braveInfoBorder
  static let toastProgressColor = UIColor.braveInfoLabel
}

class DownloadToast: Toast {
  lazy var progressView: UIView = {
    let progressView = UIView()
    progressView.backgroundColor = DownloadToastUX.toastProgressColor
    return progressView
  }()

  var percent: CGFloat = 0.0 {
    didSet {
      UIView.animate(withDuration: 0.05) {
        self.descriptionLabel.text = self.descriptionText
        self.progressWidthConstraint?.update(offset: self.toastView.frame.width * self.percent)
        self.layoutIfNeeded()
      }
    }
  }

  var combinedBytesDownloaded: Int64 {
    downloads.reduce(0, { $0 + $1.receivedBytes })
  }

  var combinedTotalBytesExpected: Int64? {
    didSet {
      updatePercent()
    }
  }

  var descriptionText: String {
    let downloadedSize = ByteCountFormatter.string(
      fromByteCount: combinedBytesDownloaded,
      countStyle: .file
    )
    let expectedSize =
      combinedTotalBytesExpected != nil
      ? ByteCountFormatter.string(fromByteCount: combinedTotalBytesExpected!, countStyle: .file)
      : nil
    let descriptionText =
      expectedSize != nil
      ? String(format: Strings.downloadProgressToastDescriptionText, downloadedSize, expectedSize!)
      : downloadedSize

    guard downloads.count > 1 else {
      return descriptionText
    }

    let fileCountDescription = String(
      format: Strings.downloadMultipleFilesToastDescriptionText,
      downloads.count
    )

    return String(
      format: Strings.downloadMultipleFilesAndProgressToastDescriptionText,
      fileCountDescription,
      descriptionText
    )
  }

  var downloads: [CWVDownloadTask] = []

  let descriptionLabel = UILabel()
  var progressWidthConstraint: Constraint?

  init(download: CWVDownloadTask, completion: @escaping (_ buttonPressed: Bool) -> Void) {
    super.init(frame: .zero)

    self.completionHandler = completion
    self.clipsToBounds = true

    self.combinedTotalBytesExpected =
      download.totalBytes != CWVDownloadSizeUnknown ? download.totalBytes : nil

    self.downloads.append(download)

    self.addSubview(createView(download.suggestedFileName, descriptionText: self.descriptionText))

    self.toastView.snp.makeConstraints { make in
      make.left.right.height.equalTo(self)
      self.animationConstraint = make.top.equalTo(self).offset(ButtonToastUX.toastHeight).constraint
    }

    self.snp.makeConstraints { make in
      make.height.equalTo(ButtonToastUX.toastHeight)
    }
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func addDownload(_ download: CWVDownloadTask) {
    downloads.append(download)

    if let combinedTotalBytesExpected = self.combinedTotalBytesExpected {
      if let totalBytesExpected = download.totalBytes != CWVDownloadSizeUnknown
        ? download.totalBytes : nil
      {
        self.combinedTotalBytesExpected = combinedTotalBytesExpected + totalBytesExpected
      }
    }
  }

  func updatePercent() {
    DispatchQueue.main.async {
      guard let combinedTotalBytesExpected = self.combinedTotalBytesExpected else {
        self.percent = 0.0
        return
      }

      self.percent = CGFloat(self.combinedBytesDownloaded) / CGFloat(combinedTotalBytesExpected)
    }
  }

  func createView(_ labelText: String, descriptionText: String) -> UIView {
    let horizontalStackView = UIStackView()
    horizontalStackView.axis = .horizontal
    horizontalStackView.alignment = .center
    horizontalStackView.spacing = ButtonToastUX.toastPadding

    let icon = UIImageView(
      image: UIImage(named: "download", in: .module, compatibleWith: nil)!.template
    )
    icon.tintColor = .white
    horizontalStackView.addArrangedSubview(icon)

    let labelStackView = UIStackView()
    labelStackView.axis = .vertical
    labelStackView.alignment = .leading

    let label = UILabel()
    label.textColor = .white
    label.font = ButtonToastUX.toastLabelFont
    label.text = labelText
    label.lineBreakMode = .byWordWrapping
    label.numberOfLines = 1
    label.adjustsFontSizeToFitWidth = true
    labelStackView.addArrangedSubview(label)

    descriptionLabel.textColor = .white
    descriptionLabel.font = ButtonToastUX.toastDescriptionFont
    descriptionLabel.text = descriptionText
    descriptionLabel.lineBreakMode = .byTruncatingTail
    labelStackView.addArrangedSubview(descriptionLabel)

    horizontalStackView.addArrangedSubview(labelStackView)

    let cancel = UIImageView(
      image: UIImage(named: "close-medium", in: .module, compatibleWith: nil)!.template
    )
    cancel.tintColor = .white
    cancel.isUserInteractionEnabled = true
    cancel.addGestureRecognizer(
      UITapGestureRecognizer(target: self, action: #selector(buttonPressed))
    )
    horizontalStackView.addArrangedSubview(cancel)

    toastView.backgroundColor = DownloadToastUX.toastBackgroundColor

    toastView.addSubview(progressView)
    toastView.addSubview(horizontalStackView)

    progressView.snp.makeConstraints { make in
      make.left.equalTo(toastView)
      make.centerY.equalTo(toastView)
      make.height.equalTo(toastView)
      progressWidthConstraint = make.width.equalTo(0.0).constraint
    }

    horizontalStackView.snp.makeConstraints { make in
      make.centerX.equalTo(toastView)
      make.centerY.equalTo(toastView)
      make.width.equalTo(toastView.snp.width).offset(-2 * ButtonToastUX.toastPadding)
    }

    return toastView
  }

  @objc func buttonPressed(_ gestureRecognizer: UIGestureRecognizer) {
    let alert = AlertController(
      title: Strings.cancelDownloadDialogTitle,
      message: Strings.cancelDownloadDialogMessage,
      preferredStyle: .alert
    )
    alert.addAction(
      UIAlertAction(title: Strings.cancelDownloadDialogResume, style: .cancel, handler: nil),
      accessibilityIdentifier: "cancelDownloadAlert.resume"
    )
    alert.addAction(
      UIAlertAction(
        title: Strings.cancelDownloadDialogCancel,
        style: .default,
        handler: { action in
          self.completionHandler?(true)
          self.dismiss(true)
        }
      ),
      accessibilityIdentifier: "cancelDownloadAlert.cancel"
    )

    viewController?.present(alert, animated: true, completion: nil)
  }

  @objc override func handleTap(_ gestureRecognizer: UIGestureRecognizer) {
    // Intentional NOOP to override superclass behavior for dismissing the toast.
  }
}
