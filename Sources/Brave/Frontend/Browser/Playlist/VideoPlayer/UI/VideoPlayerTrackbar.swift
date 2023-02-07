// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import AVKit

private class VideoSliderBar: UIControl {
  public var trackerInsets = UIEdgeInsets(top: 0.0, left: 5.0, bottom: 0.0, right: 5.0)
  public var value: CGFloat = 0.0 {
    didSet {
      // Calculating the tracker offsets based on a ratio (value) of its bounds
      trackerConstraint?.constant = boundaryView.bounds.size.width * value

      // Clamps the value between 0.0 and 1.0
      // Calculates the insets and then the offsets based on the ratio (value) of its bounds relative to its start position
      filledConstraint?.constant = value >= 1.0 ? bounds.size.width : ((bounds.size.width - (trackerInsets.left + trackerInsets.right)) * value) + trackerInsets.left
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    tracker.addGestureRecognizer(UIPanGestureRecognizer(target: self, action: #selector(onPanned(_:))))
    background.addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(onPanned(_:))))

    addSubview(background)
    addSubview(boundaryView)

    background.addSubview(filledView)
    boundaryView.addSubview(tracker)

    background.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    boundaryView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    filledView.snp.makeConstraints {
      $0.trailing.top.bottom.equalTo(background)
    }

    tracker.snp.makeConstraints {
      $0.centerY.equalTo(boundaryView.snp.centerY)
      $0.width.height.equalTo(18.0)
    }

    filledConstraint = filledView.leadingAnchor.constraint(equalTo: background.leadingAnchor).then {
      $0.isActive = true
    }

    trackerConstraint = tracker.centerXAnchor.constraint(equalTo: boundaryView.leadingAnchor).then {
      $0.isActive = true
    }
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    self.background.layer.cornerRadius = self.bounds.size.height / 2.0

    boundaryView.snp.remakeConstraints {
      $0.edges.equalToSuperview().inset(self.trackerInsets)
    }

    if self.filledConstraint?.constant ?? 0 < self.trackerInsets.left {
      self.filledConstraint?.constant = self.trackerInsets.left
    }
  }

  override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
    if tracker.bounds.size.width < 44.0 || tracker.bounds.size.height < 44.0 {
      let adjustedBounds = CGRect(x: tracker.center.x, y: tracker.center.y, width: 0.0, height: 0.0).inset(by: touchInsets)

      if adjustedBounds.contains(point) {
        return tracker
      }
    }

    let adjustedBounds = background.bounds.inset(by: touchInsets)
    if adjustedBounds.contains(point) {
      return background
    }

    return super.hitTest(point, with: event)
  }

  override func point(inside point: CGPoint, with event: UIEvent?) -> Bool {
    if tracker.bounds.size.width < 44.0 || tracker.bounds.size.height < 44.0 {
      let adjustedBounds = CGRect(x: tracker.center.x, y: tracker.center.y, width: 0.0, height: 0.0).inset(by: touchInsets)

      if adjustedBounds.contains(point) {
        return true
      }
    }

    let adjustedBounds = background.bounds.inset(by: touchInsets)
    if adjustedBounds.contains(point) {
      return true
    }

    return super.point(inside: point, with: event)
  }

  @objc
  private func onPanned(_ recognizer: UIGestureRecognizer) {
    let offset = min(boundaryView.bounds.size.width, max(0.0, recognizer.location(in: boundaryView).x))

    value = offset / boundaryView.bounds.size.width

    sendActions(for: .valueChanged)

    if recognizer.state == .cancelled || recognizer.state == .ended {
      sendActions(for: .touchUpInside)
    }
  }

  private var filledConstraint: NSLayoutConstraint?
  private var trackerConstraint: NSLayoutConstraint?

  private let touchInsets = UIEdgeInsets(top: 44.0, left: 44.0, bottom: 44.0, right: 44.0)

  private var background = UIView().then {
    $0.backgroundColor = .white
    $0.clipsToBounds = true
  }

  private var filledView = UIView().then {
    $0.backgroundColor = .black
    $0.clipsToBounds = true
  }

  private var boundaryView = UIView().then {
    $0.backgroundColor = .clear
  }

  private var tracker = UIImageView().then {
    $0.contentMode = .scaleAspectFit
    $0.isUserInteractionEnabled = true
    $0.image = UIImage(named: "playlist_video_thumb", in: .module, compatibleWith: nil)!
  }
}

protocol VideoTrackerBarDelegate: AnyObject {
  func onValueChanged(_ trackBar: VideoTrackerBar, value: CGFloat)
  func onValueEnded(_ trackBar: VideoTrackerBar, value: CGFloat)
}

class VideoTrackerBar: UIView {
  public weak var delegate: VideoTrackerBarDelegate?

  private let slider = VideoSliderBar()

  private static let formatter = DateComponentsFormatter().then {
    $0.allowedUnits = [.day, .hour, .minute, .second]
    $0.unitsStyle = .positional
    $0.zeroFormattingBehavior = [.dropLeading, .pad]
  }

  private static let minimumFormatter = DateComponentsFormatter().then {
    $0.allowedUnits = [.minute, .second]
    $0.unitsStyle = .positional
    $0.zeroFormattingBehavior = [.pad]
  }

  private let currentTimeLabel = UILabel().then {
    $0.text = "00:00"
    $0.textColor = .white
    $0.textColor = #colorLiteral(red: 0.5254901961, green: 0.5568627451, blue: 0.5882352941, alpha: 1)
    $0.font = .systemFont(ofSize: 14.0, weight: .medium)
  }

  private let endTimeLabel = UILabel().then {
    $0.text = "00:00"
    $0.textColor = .white
    $0.textColor = #colorLiteral(red: 0.5254901961, green: 0.5568627451, blue: 0.5882352941, alpha: 1)
    $0.font = .systemFont(ofSize: 14.0, weight: .medium)
  }

  public func setTimeRange(currentTime: CMTime, endTime: CMTime) {
    if CMTimeCompare(endTime, .zero) != 0,
      endTime.value > 0,
      currentTime.isValid && !currentTime.isIndefinite,
      endTime.isValid && !endTime.isIndefinite {

      slider.value = CGFloat(currentTime.value) / CGFloat(endTime.value)
      currentTimeLabel.text = VideoTrackerBar.timeToString(currentTime)
      endTimeLabel.text = "-\(VideoTrackerBar.timeToString(endTime - currentTime))"
    } else {
      slider.value = 0.0
      currentTimeLabel.text = "00:00"
      endTimeLabel.text = "00:00"
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    slider.addTarget(self, action: #selector(onValueChanged(_:)), for: .valueChanged)
    slider.addTarget(self, action: #selector(onValueEnded(_:)), for: .touchUpInside)

    addSubview(slider)
    addSubview(currentTimeLabel)
    addSubview(endTimeLabel)

    currentTimeLabel.snp.makeConstraints {
      $0.leading.equalToSuperview().inset(10.0)
      $0.top.equalToSuperview().offset(2.0)
      $0.bottom.equalTo(slider.snp.top).offset(-10.0)
    }

    endTimeLabel.snp.makeConstraints {
      $0.trailing.equalToSuperview().inset(10.0)
      $0.top.equalToSuperview().offset(2.0)
      $0.bottom.equalTo(slider.snp.top).offset(-10.0)
    }

    slider.snp.makeConstraints {
      $0.leading.trailing.equalToSuperview().inset(10.0)
      $0.bottom.equalToSuperview()
      $0.height.equalTo(2.5)
    }
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  @objc
  private func onValueChanged(_ slider: VideoSliderBar) {
    self.delegate?.onValueChanged(self, value: slider.value)
  }

  @objc
  private func onValueEnded(_ slider: VideoSliderBar) {
    self.delegate?.onValueEnded(self, value: slider.value)
  }

  public static func timeToString(_ time: CMTime) -> String {
    let totalSeconds = abs(CMTimeGetSeconds(time))
    if Int(totalSeconds) >= 3600,
      let result = formatter.string(from: totalSeconds) {
      // It is necessary to use the correct formatter because the formatter
      // can drop leading zeroes which will cause `0s` to show instead of `00:00`
      // Also if all zeroes are dropped and padded, it formats as `00`.
      // We need to display a minimum format of `00:00`
      // So we have a minimumFormatter (< 1h) and a regular formatter (>= 1h).
      return result
    } else if let result = minimumFormatter.string(from: totalSeconds) {
      return result
    }

    // If the DateFormatter cannot parse the seconds into the correct components
    // We can do so manually if necessary.
    let days = floor(totalSeconds.remainder(dividingBy: 31536000.0) / 86400.0)
    let hours = floor(totalSeconds.remainder(dividingBy: 86400.0) / 3600.0)
    let minutes = floor(totalSeconds.remainder(dividingBy: 3600.0) / 60.0)
    let seconds = floor(totalSeconds.remainder(dividingBy: 60.0))

    var result = ""
    if Int(days) > 0 {
      result += String(format: "%02zu:", Int(days))
    }

    if Int(days) > 0 || Int(hours) > 0 {
      result += String(format: "%02zu:", Int(hours))
    }

    return result + String(format: "%02zu:%02zu", Int(minutes), Int(seconds))
  }
}
