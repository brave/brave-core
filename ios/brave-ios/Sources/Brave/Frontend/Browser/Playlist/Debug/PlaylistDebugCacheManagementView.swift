// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Data
@_spi(PlaylistDebug) import Playlist
import SwiftUI

struct PlaylistDebugCacheManagementView: View {
  @State private var simulateDiskEncumbered = false
  @State private var isRunningReclamation = false
  @State private var cachedItemCount = 0
  @State private var diskUsedPercent: Double?
  @State private var isReclamationEnabled = false
  @State private var isDiskEncumbered = false
  @State private var outOfSpaceRetryCount = 0
  @State private var lastReclamationMessage: String?

  var body: some View {
    VStack(alignment: .leading, spacing: 12) {
      Text("LRU Cache Management")
        .font(.headline)

      Group {
        Divider()
        statusRow("Reclamation enabled", value: isReclamationEnabled ? "Yes" : "No")
        statusRow(
          "Disk encumbered",
          value: isDiskEncumbered ? "Yes" : "No",
          secondary: simulateDiskEncumbered ? "(simulated)" : nil
        )
        if let diskUsedPercent {
          statusRow("Disk used", value: String(format: "%.1f%%", diskUsedPercent))
        }
        statusRow("Cached items", value: "\(cachedItemCount)")
        statusRow("Out-of-space retries", value: "\(outOfSpaceRetryCount)")
      }
      .font(.subheadline)

      if let lastReclamationMessage {
        Text(lastReclamationMessage)
          .font(.caption)
          .foregroundStyle(.secondary)
      }

      Toggle("Simulate disk ≥90% full", isOn: $simulateDiskEncumbered)
        .onChange(of: simulateDiskEncumbered) { _, newValue in
          Task { @MainActor in
            PlaylistManager.shared.debugForceDiskSpaceEncumbered = newValue
            refreshStatus()
          }
        }

      Grid {
        GridRow {
          debugButton(
            title: "Reclaim Space",
            isLoading: isRunningReclamation,
            isDisabled: isRunningReclamation || !isReclamationEnabled
          ) {
            isRunningReclamation = true
            lastReclamationMessage = nil
            Task { @MainActor in
              let beforeCount = PlaylistManager.shared.debugCachedItemCount()
              await PlaylistManager.shared.debugRunCacheReclamation()
              let afterCount = PlaylistManager.shared.debugCachedItemCount()
              lastReclamationMessage = "Evicted \(beforeCount - afterCount) cached item(s)."
              isRunningReclamation = false
              refreshStatus()
            }
          }

          debugButton(title: "Reset Tracking") {
            Task { @MainActor in
              PlaylistManager.shared.debugResetOutOfSpaceRetryTracking()
              refreshStatus()
            }
          }
        }

        GridRow {
          debugButton(title: "Reset Migration") {
            Migration.debugResetPlaylistLastPlayedDateMigration()
          }

          debugButton(title: "Refresh Status") {
            refreshStatus()
          }
        }
      }
      .padding(.top, 40)
    }
    .padding(.horizontal, 24)
    .frame(maxWidth: .infinity, alignment: .leading)
    .onAppear {
      refreshStatus()
    }
  }

  @ViewBuilder
  private func statusRow(_ title: String, value: String, secondary: String? = nil) -> some View {
    HStack {
      Text(title)
      Spacer()
      HStack(spacing: 4) {
        Text(value)
        if let secondary {
          Text(secondary)
            .foregroundStyle(.secondary)
        }
      }
    }
  }

  @ViewBuilder
  private func debugButton(
    title: String,
    isLoading: Bool = false,
    isDisabled: Bool = false,
    action: @escaping () -> Void
  ) -> some View {
    Button(action: action) {
      Text(title)
        .foregroundStyle(isLoading ? Color.clear : Color.white)
        .padding()
        .frame(maxWidth: .infinity)
        .background(
          ContainerRelativeShape()
            .fill(Color(braveSystemName: isDisabled ? .buttonDisabled : .primary70))
            .shadow(color: Color.black.opacity(0.25), radius: 8.0, x: 0.0, y: 1.0)
        )
        .containerShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
        .overlay {
          if isLoading {
            ProgressView()
              .progressViewStyle(.circular)
              .tint(Color(braveSystemName: .primary70))
              .padding()
          }
        }
    }
    .disabled(isDisabled)
  }

  @MainActor
  private func refreshStatus() {
    let manager = PlaylistManager.shared
    cachedItemCount = manager.debugCachedItemCount()
    diskUsedPercent = manager.debugDiskUsedPercent()
    isReclamationEnabled = manager.debugIsCacheReclamationEnabled
    isDiskEncumbered = manager.isDiskSpaceEncumbered()
    outOfSpaceRetryCount = manager.debugOutOfSpaceRetryItemCount
    simulateDiskEncumbered = manager.debugForceDiskSpaceEncumbered
  }
}
