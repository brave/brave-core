// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import CoreHaptics
import Lottie
import SwiftUI
import os.log

struct FocusAdTrackerSliderContentView: View {
  @Environment(\.colorScheme) private var colorScheme

  @State private var progress: CGFloat = 0.75
  @State private var opacity = 0.0

  var body: some View {
    SwipeDifferenceView(progress: $progress) {
      ZStack {
        Image("focus-website-ads", bundle: .module)
        LottieView(
          animation: .named(
            colorScheme == .dark ? "moving-ads-dark" : "moving-ads-light",
            bundle: .module,
            subdirectory: "LottieAssets/\(Locale.current.region?.identifier == "JP" ? "ja" : "en")"
          )
        )
        .playing(loopMode: .loop)
        .resizable()
        .opacity(opacity)
      }
    } trailing: {
      Image("focus-website-noads", bundle: .module)
        .aspectRatio(contentMode: .fit)
    }
    .frame(maxWidth: .infinity)
    .onAppear {
      withAnimation(.easeInOut(duration: 0.15).delay(2.0)) {
        opacity = 1.0
      }

      Timer.scheduledTimer(withTimeInterval: 2.5, repeats: false) { _ in
        withAnimation(.easeInOut(duration: 1.5)) {
          progress = 0.85
        }
        Timer.scheduledTimer(withTimeInterval: 1.5, repeats: false) { _ in
          withAnimation(.easeInOut(duration: 1.5)) {
            progress = 0.15
          }
          Timer.scheduledTimer(withTimeInterval: 1.5, repeats: false) { _ in
            withAnimation(.easeInOut(duration: 1.5)) {
              progress = 0.25
            }
          }
        }
      }
    }
  }
}

struct FocusVideoAdSliderContentView: View {
  @Environment(\.colorScheme) private var colorScheme

  var body: some View {
    LottieView(
      animation: .named(
        colorScheme == .dark ? "novideo-ads-dark" : "novideo-ads-light",
        bundle: .module,
        subdirectory: "LottieAssets/\(Locale.current.region?.identifier == "JP" ? "ja" : "en")"
      )
    )
    .playing(loopMode: .loop)
    .resizable()
    .aspectRatio(contentMode: .fill)
  }
}

struct SwipeDifferenceView<Leading: View, Trailing: View>: View {
  @Environment(\.layoutDirection) private var layoutDirection
  @Environment(\.scenePhase) var scenePhase

  private enum HapticsLevel: Float {
    case none = 0
    case low = 0.20
    case lowMedium = 0.35
    case medium = 0.50
    case highMedium = 0.65
    case high = 0.80
    case intense = 1.0
  }

  @GestureState private var initialProgress: CGFloat?

  @Binding var progress: CGFloat

  // CoreHaptics Engine and Player for slider progress value
  @State private var hapticsEngine: CHHapticEngine?
  @State private var hapticsPlayer: CHHapticPatternPlayer?
  @State private var hapticsLevel: HapticsLevel = .intense
  @State private var hapticsTimer: Timer?

  var leading: Leading
  var trailing: Trailing

  init(
    progress: Binding<CGFloat>,
    @ViewBuilder leading: () -> Leading,
    @ViewBuilder trailing: () -> Trailing
  ) {
    self._progress = progress
    self.leading = leading()
    self.trailing = trailing()
  }

  var isRTL: Bool {
    layoutDirection == .rightToLeft
  }

  @ViewBuilder private var actualLeading: some View {
    if isRTL {
      trailing
    } else {
      leading
    }
  }

  @ViewBuilder private var actualTrailing: some View {
    if isRTL {
      leading
    } else {
      trailing
    }
  }

  var body: some View {
    ZStack {
      actualLeading
      actualTrailing
        .mask {
          GeometryReader { proxy in
            Color.black
              .frame(width: proxy.size.width * (isRTL ? progress : (1 - progress)))
              .frame(
                maxWidth: .infinity,
                maxHeight: .infinity,
                alignment: isRTL ? .leading : .trailing
              )
          }
        }
    }
    .onAppear {
      Task.delayed(bySeconds: 1.5) { @MainActor in
        withAnimation(.easeInOut(duration: 1.0)) {
          prepareSliderHaptics()
          prepareRandomHaptics()
        }
      }
    }
    .onDisappear(perform: {
      stopAllHaptics()
    })
    .onChange(of: progress) { newValue in
      hapticsLevel = determineHapticIntensityLevel(from: newValue)
    }
    .onChange(of: hapticsLevel) { newValue in
      createContinousHapticFeedback(intensity: newValue)
    }
    .onChange(of: scenePhase) { newValue in
      switch newValue {
      case .active:
        prepareSliderHaptics()
        prepareRandomHaptics()
      case .inactive, .background:
        stopAllHaptics()
      @unknown default:
        stopAllHaptics()
      }
    }
    .overlay {
      // Fake "grabber"/splitter
      GeometryReader { proxy in
        Color(braveSystemName: .textPrimary)
          .frame(width: 4)
          .shadow(radius: 8)
          .overlay {
            Image("focus-slider-knob", bundle: .module)
          }
          .offset(x: (proxy.size.width * progress))
      }
    }
    .overlay {
      // Drag gesture view
      GeometryReader { proxy in
        Color.clear
          .contentShape(Rectangle())
          .gesture(
            DragGesture()
              .updating(
                $initialProgress,
                body: { value, state, _ in
                  // Sets initialProgress when the gesture is active
                  if state == nil {
                    state = progress
                  }
                }
              )
              .onChanged { value in
                guard let initialProgress else { return }
                withAnimation(.interactiveSpring) {
                  progress = max(
                    0,
                    min(1, initialProgress + (value.translation.width) / proxy.size.width)
                  )
                  if isRTL {
                    progress = 1 - progress
                  }
                }
              }
          )
      }
    }
    .padding()
  }

  private func prepareSliderHaptics() {
    // Check Hardware is capable
    guard CHHapticEngine.capabilitiesForHardware().supportsHaptics else { return }

    // Create Haptics Engine
    do {
      hapticsEngine = try CHHapticEngine()
      try hapticsEngine?.start()
    } catch {
      Logger.module.debug(
        "[Focus Onboarding] - There was an error creating the engine: \(error.localizedDescription)"
      )
    }
  }

  private func prepareRandomHaptics() {
    if hapticsTimer == nil {
      hapticsTimer = Timer.scheduledTimer(withTimeInterval: 0.25, repeats: true) { _ in
        createRandomHapticEffect()
      }
    }

    hapticsTimer?.tolerance = 0.1
  }

  private func createContinousHapticFeedback(intensity: HapticsLevel) {
    do {
      try hapticsPlayer?.cancel()

      let continuousPattern = try CHHapticPattern(
        events: [
          CHHapticEvent(
            eventType: .hapticContinuous,
            parameters: [
              CHHapticEventParameter(parameterID: .hapticIntensity, value: intensity.rawValue),
              CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.5),
            ],
            relativeTime: 0,
            duration: 100
          )
        ],
        parameters: []
      )

      // Create pattern player
      hapticsPlayer = try hapticsEngine?.makePlayer(with: continuousPattern)
      try hapticsPlayer?.start(atTime: CHHapticTimeImmediate)
    } catch let error {
      Logger.module.debug(
        "[Focus Onboarding] - Error creating haptic pattern: \(error.localizedDescription)"
      )
    }
  }

  private func stopAllHaptics() {
    hapticsTimer?.invalidate()
    hapticsTimer = nil

    Task { @MainActor in
      do {
        try hapticsPlayer?.cancel()
        try await hapticsEngine?.stop()
      } catch let error {
        Logger.module.debug(
          "[Focus Onboarding] - Error stopping haptic engine: \(error.localizedDescription)"
        )
      }
    }
  }

  private func determineHapticIntensityLevel(from progress: CGFloat) -> HapticsLevel {
    let progressPercentage = Int(progress * 100)

    switch progressPercentage {
    case 0..<5:
      return .none
    case 5..<25:
      return .low
    case 25..<41:
      return .lowMedium
    case 41..<61:
      return .medium
    case 61..<75:
      return .highMedium
    case 75..<95:
      return .high
    case 95..<100:
      return .intense
    default:
      return .high
    }
  }

  private func createRandomHapticEffect() {
    guard progress > 0.05 else {
      return
    }

    let randomEffect = Int.random(in: 0..<8)

    switch randomEffect {
    case 1:
      UINotificationFeedbackGenerator().vibrate(style: .warning)
    case 2:
      UIImpactFeedbackGenerator(style: .medium).impactOccurred()
    case 3:
      UINotificationFeedbackGenerator().vibrate(style: .error)
    case 4:
      UISelectionFeedbackGenerator().vibrate()
    case 5:
      UINotificationFeedbackGenerator().vibrate(style: .success)
    case 6:
      UIImpactFeedbackGenerator(style: .heavy).impactOccurred()
    case 7:
      UIImpactFeedbackGenerator(style: .light).impactOccurred()
    default:
      UINotificationFeedbackGenerator().vibrate(style: .error)
    }
  }
}

#if DEBUG
#Preview("AdTrackerLTR") {
  FocusAdTrackerSliderContentView()
}

#Preview("AdTrackerRTL") {
  FocusAdTrackerSliderContentView()
    .environment(\.layoutDirection, .rightToLeft)
}

#Preview("VideoTracker") {
  FocusVideoAdSliderContentView()
    .aspectRatio(contentMode: .fit)
}
#endif
