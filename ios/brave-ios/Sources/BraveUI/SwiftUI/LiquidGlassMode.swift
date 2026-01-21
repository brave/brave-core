// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

/// Utility for checking SwiftUI capabilities and system features
public enum LiquidGlassMode {
  /// Determines if Liquid Glass is enabled on iOS 26+
  public static var isEnabled: Bool {
    if #available(iOS 26, *) {
      let isCompatabilityModeEnabled =
        Bundle.main.infoDictionary?["UIDesignRequiresCompatibility"] as? Bool == true
      if isCompatabilityModeEnabled {
        let key = "com.apple.Swi\("ftUI.IgnoreSolar")iumOptOut"
        return UserDefaults.standard.bool(forKey: key)
      }
      return true
    }
    return false
  }
}

extension UIView {
  /// Returns the appropriate layout guide for horizontal constraints, accounting for iOS 26+ Liquid Glass mode
  /// When Liquid Glass is enabled, returns a layout guide with corner adaptation for horizontal constraints suitable for window controls
  public var liquidGlassHorizontalSafeAreaLayoutGuide: UILayoutGuide {
    if #available(iOS 26.0, *), LiquidGlassMode.isEnabled {
      return layoutGuide(for: .safeArea(cornerAdaptation: .horizontal))
    }
    return safeAreaLayoutGuide
  }
}

struct LiquidGlassLayoutTrafficLightPadding: ViewModifier {
  @State private var trafficLightsVisible = false
  @State private var layoutGuideInset: CGFloat = 0
  let padding: CGFloat = 8

  var explicitInset: CGFloat? = nil
  var leadingInset: CGFloat { explicitInset ?? layoutGuideInset + padding }

  init(_ explicitInset: CGFloat? = nil) {
    self.explicitInset = explicitInset
  }
  func body(content: Content) -> some View {
    if #available(iOS 26.0, *) {
      content
        .padding(.leading, trafficLightsVisible ? leadingInset : 0)
        .background(
          TrafficLightDetector { visible, inset in
            withAnimation(.easeInOut(duration: 0.25)) {
              trafficLightsVisible = visible
              layoutGuideInset = inset
            }
          }
        )
    } else {
      content
    }
  }
}

extension View {
  public func liquidGlassLayoutTrafficLightPadding(_ inset: CGFloat? = nil) -> some View {
    modifier(LiquidGlassLayoutTrafficLightPadding(inset))
  }
}

struct TrafficLightDetector: UIViewControllerRepresentable {
  let onVisibilityChange: (Bool, CGFloat) -> Void

  func makeUIViewController(context: Context) -> TrafficLightDetectorViewController {
    TrafficLightDetectorViewController(onVisibilityChange: onVisibilityChange)
  }

  func updateUIViewController(
    _ uiViewController: TrafficLightDetectorViewController,
    context: Context
  ) {}
}

class TrafficLightDetectorViewController: UIViewController {
  var onVisibilityChange: ((Bool, CGFloat) -> Void)?
  private var lastVisibilityState: Bool?
  private var displayLink: CADisplayLink?

  init(onVisibilityChange: ((Bool, CGFloat) -> Void)?) {
    self.onVisibilityChange = onVisibilityChange
    super.init(nibName: nil, bundle: nil)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    startMonitoring()
  }

  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)
    stopMonitoring()
  }

  private func startMonitoring() {
    displayLink?.invalidate()
    displayLink = CADisplayLink(target: self, selector: #selector(checkTrafficLights))
    displayLink?.preferredFramesPerSecond = 10  // Check 10 times per second
    displayLink?.add(to: .main, forMode: .common)

    checkTrafficLights()
  }

  private func stopMonitoring() {
    displayLink?.invalidate()
    displayLink = nil
  }

  @objc private func checkTrafficLights() {
    guard let windowScene = view.window?.windowScene else {
      updateVisibility(false)
      return
    }

    guard #available(iOS 26.0, *) else {
      updateVisibility(false)
      return
    }

    let windowBounds = windowScene.coordinateSpace.bounds
    let screenBounds = windowScene.screen.bounds

    // Traffic lights are visible when window is NOT maximized. That is, app is in windowing mode
    let threshold: CGFloat = 1.0
    let isMaximized =
      abs(windowBounds.width - screenBounds.width) < threshold
      && abs(windowBounds.height - screenBounds.height) < threshold
    let trafficLightsVisible = !isMaximized
    updateVisibility(trafficLightsVisible)
  }

  private func updateVisibility(_ visible: Bool) {
    if let onVisibilityChange, lastVisibilityState != visible {
      lastVisibilityState = visible
      let inset =
        view.bounds.width > 0 ? view.liquidGlassHorizontalSafeAreaLayoutGuide.layoutFrame.minX : 0
      onVisibilityChange(visible, inset)
    }
  }

  deinit {
    stopMonitoring()
  }
}

private struct LiquidGlassHorizontalInsetMeasurer: UIViewRepresentable {
  let onInsetsMeasured: (CGFloat, CGFloat) -> Void

  func makeUIView(context: Context) -> LiquidGlassHorizontalInsetMeasuringView {
    LiquidGlassHorizontalInsetMeasuringView(onInsetsMeasured: onInsetsMeasured)
  }

  func updateUIView(_ uiView: LiquidGlassHorizontalInsetMeasuringView, context: Context) {
    uiView.onInsetsMeasured = onInsetsMeasured
    uiView.setNeedsLayout()
  }
}

private class LiquidGlassHorizontalInsetMeasuringView: UIView {
  var onInsetsMeasured: (CGFloat, CGFloat) -> Void

  init(onInsetsMeasured: @escaping (CGFloat, CGFloat) -> Void) {
    self.onInsetsMeasured = onInsetsMeasured
    super.init(frame: .zero)
    backgroundColor = .clear
    isUserInteractionEnabled = false
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    let layoutGuide = liquidGlassHorizontalSafeAreaLayoutGuide
    let leading = layoutGuide.layoutFrame.minX
    let trailing = bounds.width - layoutGuide.layoutFrame.maxX
    onInsetsMeasured(leading, trailing)
  }
}
