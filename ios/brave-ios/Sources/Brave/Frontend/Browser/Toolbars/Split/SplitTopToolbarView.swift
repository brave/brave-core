// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveStrings
import BraveUI
import Combine
import Shared
import SnapKit
import SpeechRecognition
import SwiftUI
import UIKit

/// The top row of the split layout: the location bar & the shields button
struct SplitTopToolbarView: View {
  var toolbarState: BrowserToolbarState
  var privateBrowsingManager: PrivateBrowsingManager
  var speechRecognizer: SpeechRecognizer

  init(
    toolbarState: BrowserToolbarState,
    privateBrowsingManager: PrivateBrowsingManager,
    speechRecognizer: SpeechRecognizer
  ) {
    self.toolbarState = toolbarState
    self.privateBrowsingManager = privateBrowsingManager
    self.speechRecognizer = speechRecognizer
  }

  @ScaledMetric(relativeTo: .body) private var shieldsButtonSize = 24

  var body: some View {
    HStack(spacing: 8) {
      SplitLocationBar(
        toolbarState: toolbarState,
        privateBrowsingManager: privateBrowsingManager,
        speechRecognizer: speechRecognizer
      )
      Button {
        // TODO: Forward to toolbar commands
      } label: {
        shieldsImage
          .resizable()
          .aspectRatio(contentMode: .fit)
          .frame(width: shieldsButtonSize, height: shieldsButtonSize)
      }
      .accessibilityLabel(Strings.bravePanel)
      .accessibilityIdentifier("urlBar-shieldsButton")
    }
    .dynamicTypeSize(...DynamicTypeSize.xxLarge)
    .padding(.horizontal, 12)
    .padding(.vertical, 8)
    .frame(maxWidth: .infinity)
  }

  private var shieldsImage: Image {
    // TODO: Reflect the shields state of the site once shields are forwarded through the toolbar
    // state
    if let url = toolbarState.displayedURL, url.isWebPage(includeDataURIs: false),
      !url.isLocal, !url.isNewTabURL
    {
      return Image(sharedName: "brave.logo")
    }
    return Image(sharedName: "brave.logo.greyscale")
  }
}

/// A `TabLocationView` inside of a `LocationContainerView` which displays the URL & load state of
/// `BrowserToolbarState`
private struct SplitLocationBar: UIViewRepresentable {
  var toolbarState: BrowserToolbarState
  var privateBrowsingManager: PrivateBrowsingManager
  var speechRecognizer: SpeechRecognizer

  func makeUIView(context: Context) -> SplitLocationBarView {
    SplitLocationBarView(
      toolbarState: toolbarState,
      privateBrowsingManager: privateBrowsingManager,
      speechRecognizer: speechRecognizer
    )
  }

  func updateUIView(_ uiView: SplitLocationBarView, context: Context) {
  }

  func sizeThatFits(
    _ proposal: ProposedViewSize,
    uiView: SplitLocationBarView,
    context: Context
  ) -> CGSize? {
    let width = proposal.width ?? UIView.layoutFittingCompressedSize.width
    let fittingHeight = uiView.locationView.systemLayoutSizeFitting(
      CGSize(width: width, height: UIView.layoutFittingCompressedSize.height),
      withHorizontalFittingPriority: .required,
      verticalFittingPriority: .fittingSizeLevel
    ).height
    return CGSize(width: width, height: max(TopToolbarView.UX.locationHeight, fittingHeight))
  }
}

private final class SplitLocationBarView: LocationContainerView {
  let locationView: TabLocationView

  private let toolbarState: BrowserToolbarState
  private let privateBrowsingManager: PrivateBrowsingManager
  private var appliedLoadingProgress: Float?
  private var cancellables: Set<AnyCancellable> = []

  init(
    toolbarState: BrowserToolbarState,
    privateBrowsingManager: PrivateBrowsingManager,
    speechRecognizer: SpeechRecognizer
  ) {
    self.toolbarState = toolbarState
    self.privateBrowsingManager = privateBrowsingManager
    self.locationView = TabLocationView(
      speechRecognizer: speechRecognizer,
      privateBrowsingManager: privateBrowsingManager
    )
    super.init(frame: .zero)

    locationView.readerModeState = .unavailable
    locationView.translationState = .unavailable
    locationView.layer.cornerRadius = TopToolbarView.UX.textFieldCornerRadius
    locationView.layer.cornerCurve = .continuous
    locationView.clipsToBounds = true
    // TODO: Forward location view actions (tap, reload, reader mode, etc.) to toolbar commands.
    // TODO: Drag interaction & context menu interaction.

    contentView.addSubview(locationView)
    locationView.snp.makeConstraints {
      $0.edges.equalToSuperview()
      $0.height.greaterThanOrEqualTo(TopToolbarView.UX.locationHeight)
    }

    privateBrowsingManager.$isPrivateBrowsing
      .removeDuplicates()
      .receive(on: RunLoop.main)
      .sink { [weak self] _ in self?.updateColors() }
      .store(in: &cancellables)
    updateColors()

    if #unavailable(iOS 26) {
      startObservingProperties()
    }
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError()
  }

  private func updateColors() {
    contentView.backgroundColor = privateBrowsingManager.browserColors.containerBackground
  }

  @available(iOS 26.0, *)
  override func updateProperties() {
    super.updateProperties()
    updateObservedProperties()
  }

  private func updateObservedProperties() {
    locationView.url = toolbarState.displayedURL

    let secureContentState = toolbarState.secureContentState
    if locationView.secureContentState != secureContentState {
      locationView.secureContentState = secureContentState
    }
    let isLoading = toolbarState.isLoading
    if locationView.loading != isLoading {
      locationView.loading = isLoading
    }
    let loadingProgress = toolbarState.loadingProgress
    if appliedLoadingProgress != loadingProgress {
      // The progress bar hides itself once loading completes, so only apply changes to avoid
      // redisplaying a completed load
      appliedLoadingProgress = loadingProgress
      if let loadingProgress {
        locationView.progressBar.alpha = 1
        locationView.progressBar.isHidden = false
        locationView.progressBar.setProgress(loadingProgress, animated: true)
      } else {
        locationView.progressBar.isHidden = true
        locationView.progressBar.setProgress(0, animated: false)
      }
    }
    let readerModeState = toolbarState.readerModeState
    if locationView.readerModeState != readerModeState {
      locationView.readerModeState = readerModeState
    }
    let translationState = toolbarState.translationState
    if locationView.translationState != translationState {
      locationView.translationState = translationState
    }
    let playlistButtonState = toolbarState.playlistButtonState
    if locationView.playlistButton.buttonState != playlistButtonState {
      locationView.playlistButton.buttonState = playlistButtonState
    }
    let walletButtonState = toolbarState.walletButtonState
    if locationView.walletButton.buttonState != walletButtonState {
      locationView.walletButton.buttonState = walletButtonState
    }
    // The wallet button takes priority over the playlist button
    locationView.walletButton.stackViewAnimationSafeIsHidden = walletButtonState == .inactive
    locationView.playlistButton.stackViewAnimationSafeIsHidden =
      walletButtonState != .inactive || playlistButtonState == .none
  }

  @available(iOS, introduced: 18, obsoleted: 26, message: "Use updateProperties directly")
  private func startObservingProperties() {
    withObservationTracking {
      updateObservedProperties()
    } onChange: { [weak self] in
      DispatchQueue.main.async {
        MainActor.assumeIsolated {
          self?.startObservingProperties()
        }
      }
    }
  }
}
