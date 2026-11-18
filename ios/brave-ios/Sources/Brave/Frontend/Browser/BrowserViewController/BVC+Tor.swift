// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
import BraveStrings
import BraveTorUI
import BrowserMenu
import DesignSystem
import Preferences
import SwiftUI
import UIKit
import Web

enum TorMenuPolicy {
  static let allowedActions: Set<Action.Identifier> = [
    .bookmarks, .history, .downloads, .addBookmark,
    .share, .findInPage, .shredData, .displaySecurityCertificate, .addFavourites,
    .copyCleanLink, .toggleReaderMode, .pageZoom, .print, .createPDF,
    .toggleNightMode, .requestDesktopSite,
  ]
}

extension BrowserViewController {
  func presentTorMenu(from sourceView: UIView, automaticallyPresented: Bool = false) {
    guard let tab = tabManager.selectedTab as? TorTabState else { return }
    clearPageZoomDialog()
    let url = tab.visibleURL.flatMap { ["http", "https"].contains($0.scheme ?? "") ? $0 : nil }
    let activities = url.map { url in
      makeShareActivities(
        url: url, tab: tab,
        syncAPI: profileController.syncAPI, sendTabAPI: profileController.sendTabAPI,
        feedDataSource: nil, isBraveNewsAvailable: false,
        source: .init(view: sourceView, rect: sourceView.bounds, arrowDirection: [.up, .down]),
        callbacks: .init(
          onToggleReaderMode: { [weak tab] in tab?.torPageTools?.toggleReaderMode() },
          onDisplayPageZoom: { [weak self] in self?.displayPageZoomDialog() },
          onDisplayCertificate: { [weak self] in self?.displayPageCertificateInfo() }
        )
      )
    } ?? []
    presentBrowserMenu(from: sourceView, activities: activities, tab: tab,
                       pageURL: url,
                       initiallyShowsConnectionSection: automaticallyPresented)
  }

  func observeTorMenuStatus() {
    guard torMenuCancellable == nil else { return }
    torMenuCancellable = TorBrowsingSession.shared.statePublisher.sink { [weak self] state in
      guard let self else { return }
      guard case .connecting = state,
        let tab = self.tabManager.selectedTab, tab.isTor, tab.isLoading,
        let url = tab.visibleURL, ["http", "https"].contains(url.scheme?.lowercased() ?? "")
      else {
        self.cancelTorMenuPopup()
        return
      }
      // Polling bootstrap updates also catches a page requested after opening a blank tab.
      guard self.torMenuPopupTabID != tab.id || self.torMenuPopupURL != url else { return }
      self.cancelTorMenuPopup()
      self.torMenuPopupTabID = tab.id
      self.torMenuPopupURL = url
      self.torMenuPopupTask = Task { [weak self, weak tab] in
        // Give this page five seconds to obtain a Tor connection.
        do { try await Task.sleep(nanoseconds: 5_000_000_000) } catch { return }
        guard let self, let tab, !Task.isCancelled,
          !Preferences.Privacy.disableTorAutomaticPopup.value,
          self.tabManager.selectedTab === tab, tab.isLoading, tab.visibleURL == url,
          self.viewIfLoaded?.window != nil,
          self.currentScene?.activationState == .foregroundActive,
          self.presentedViewController == nil
        else { return }
        guard case .connecting = TorBrowsingSession.shared.state else { return }
        self.presentTorMenu(
          from: self.toolbar?.menuButton ?? self.topToolbar.menuButton,
          automaticallyPresented: true
        )
      }
    }
  }

  private func cancelTorMenuPopup() {
    torMenuPopupTask?.cancel()
    torMenuPopupTask = nil
    torMenuPopupTabID = nil
    torMenuPopupURL = nil
  }
}

@MainActor
struct TorMenuStatusView: View {
  @State private var state = TorBrowsingSession.shared.state
  @State private var stage = TorBrowsingSession.shared.bootstrapSummary
  @State private var isReconnecting = TorBrowsingSession.shared.isReconnecting
  @ObservedObject private var disablePopup = Preferences.Privacy.disableTorAutomaticPopup

  var body: some View {
    VStack(alignment: .leading, spacing: 12) {
      HStack(spacing: 6) {
        Image(braveSystemName: "leo.product.tor")
          .font(.subheadline)
          .accessibilityHidden(true)
        Text(Strings.Tor.connectionStatus).font(.headline)
      }
      HStack(spacing: 12) {
        statusGlyph.frame(width: 44, height: 44).accessibilityHidden(true)
        VStack(alignment: .leading, spacing: 4) {
          Text(statusText).font(.subheadline)
          if case .connecting(let progress) = state {
            Text("\(min(progress, 99))%").font(.caption.monospacedDigit())
          }
        }
      }
      Button(Strings.Tor.reloadConnection) { TorBrowsingSession.shared.reconnect() }
      Button {
        disablePopup.value.toggle()
      } label: {
        HStack(spacing: 4) {
          Text(Strings.Tor.disableAutomaticPopup)
          Image(braveSystemName: "leo.check.normal")
            .font(.caption2)
            .opacity(disablePopup.value ? 1 : 0)
            .accessibilityHidden(true)
        }
        .font(.caption)
        .foregroundStyle(.secondary)
        .padding(.vertical, 8)
        .contentShape(.rect)
      }
      .buttonStyle(.plain)
      .accessibilityAddTraits(disablePopup.value ? .isSelected : [])
    }
    .padding()
    .frame(maxWidth: .infinity, alignment: .leading)
    .background(Color(uiColor: .secondarySystemGroupedBackground), in: .rect(cornerRadius: 14))
    .onReceive(TorBrowsingSession.shared.statePublisher) {
      state = $0
      stage = TorBrowsingSession.shared.bootstrapSummary
      isReconnecting = TorBrowsingSession.shared.isReconnecting
    }
  }

  @ViewBuilder private var statusGlyph: some View {
    switch state {
    case .connected:
      Image(systemName: "checkmark.circle.fill").font(.largeTitle).foregroundStyle(.green)
    case .unavailable:
      Image(systemName: "xmark.circle.fill").font(.largeTitle).foregroundStyle(.red)
    case .waiting:
      ProgressView()
    case .connecting(let progress):
      ZStack {
        Circle().stroke(.secondary.opacity(0.2), lineWidth: 4)
        Circle().trim(from: 0, to: CGFloat(min(progress, 99)) / 100)
          .stroke(.tint, style: StrokeStyle(lineWidth: 4, lineCap: .round))
          .rotationEffect(.degrees(-90))
        ProgressView().controlSize(.mini)
      }
    }
  }

  private var statusText: String {
    switch state {
    case .connected: Strings.Tor.connected
    case .unavailable: Strings.Tor.unavailable
    case .waiting: Strings.Tor.waiting
    case .connecting(let progress):
      if isReconnecting {
        Strings.Tor.reconnecting
      } else if progress >= 100 {
        Strings.Tor.finishingConnection
      } else {
        stage.isEmpty ? Strings.Tor.connecting : stage
      }
    }
  }
}
