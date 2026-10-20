// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferEngine
import DesignSystem
import Strings
import SwiftUI

/// The "Brave Transfer" share extension's UI (design plan §5.3): the list of
/// connected computers, then waiting, progress and the result, or the reason
/// the items go to the app.
public struct TransferShareView: View {
  @ObservedObject var model: TransferShareModel

  public init(model: TransferShareModel) {
    self.model = model
  }

  public var body: some View {
    NavigationStack {
      content
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color(braveSystemName: .pageBackground))
        .navigationTitle(Strings.BraveTransfer.shareTitle)
        .navigationBarTitleDisplayMode(.inline)
        .toolbar {
          ToolbarItem(placement: .cancellationAction) {
            if !isFinished {
              Button(Strings.BraveTransfer.shareCancel) { model.close() }
            }
          }
          ToolbarItem(placement: .confirmationAction) {
            if isFinished {
              Button(Strings.BraveTransfer.shareDone) { model.close() }
            }
          }
        }
    }
  }

  private var isFinished: Bool {
    if case .finished = model.snapshot.phase { return true }
    return false
  }

  @ViewBuilder
  private var content: some View {
    if let problem = model.problem {
      message(
        symbol: "exclamationmark.triangle",
        text: problem == .nothingToSend
          ? Strings.BraveTransfer.shareNothingToSend : Strings.BraveTransfer.handoffFailed
      )
    } else {
      switch model.snapshot.phase {
      case .preparing:
        progressMessage(Strings.BraveTransfer.sharePreparing)
      case .searching:
        searching
      case .choosing:
        deviceList
      case .sending:
        sending
      case .finished(let state):
        finished(state)
      case .handoff(let reason):
        handoff(reason)
      }
    }
  }

  private var searching: some View {
    VStack(spacing: 16) {
      Spacer()
      ProgressView()
      Text(Strings.BraveTransfer.shareSearching)
        .font(.headline)
        .foregroundStyle(Color(braveSystemName: .textPrimary))
      Text(Strings.BraveTransfer.shareEmptyState)
        .font(.subheadline)
        .multilineTextAlignment(.center)
        .foregroundStyle(Color(braveSystemName: .textSecondary))
      Spacer()
      continueInBraveButton
    }
    .padding(24)
  }

  private var deviceList: some View {
    VStack(spacing: 0) {
      List {
        Section {
          ForEach(model.snapshot.devices, id: \.id) { device in
            Button {
              model.send(to: device.id)
            } label: {
              HStack(spacing: 12) {
                Image(systemName: TransferFormatting.symbol(for: device.type))
                  .font(.title3)
                  .frame(width: 32)
                  .foregroundStyle(Color(braveSystemName: .iconInteractive))
                  .accessibilityHidden(true)
                Text(device.name)
                  .foregroundStyle(Color(braveSystemName: .textPrimary))
                Spacer()
                if !model.snapshot.isReadyToSend {
                  ProgressView()
                }
              }
            }
            .disabled(!model.snapshot.isReadyToSend)
          }
        } header: {
          Text(Strings.BraveTransfer.shareDevicesHeader)
        } footer: {
          if model.snapshot.lastSendResult == .deviceUnavailable {
            Text(Strings.BraveTransfer.shareDeviceUnavailable)
              .foregroundStyle(Color(braveSystemName: .systemfeedbackErrorText))
          } else {
            Text(Strings.BraveTransfer.shareEmptyState)
          }
        }
      }
      .scrollContentBackground(.hidden)
      continueInBraveButton
        .padding(24)
    }
  }

  private var sending: some View {
    let transfer = model.snapshot.transfer
    let name = transfer?.deviceName ?? ""
    let waiting = transfer.map { $0.state == .waitingForResponse } ?? true
    return VStack(spacing: 16) {
      Spacer()
      Image(systemName: "arrow.up.circle")
        .font(.system(size: 44))
        .foregroundStyle(Color(braveSystemName: .iconInteractive))
        .accessibilityHidden(true)
      Text(
        String.localizedStringWithFormat(
          waiting
            ? Strings.BraveTransfer.shareWaitingForAccept : Strings.BraveTransfer.shareSending,
          name
        )
      )
      .font(.headline)
      .multilineTextAlignment(.center)
      .foregroundStyle(Color(braveSystemName: .textPrimary))
      if let transfer, !waiting, transfer.bytesTotal > 0 {
        ProgressView(
          value: Double(transfer.bytesTransferred),
          total: Double(transfer.bytesTotal)
        )
        Text(
          "\(TransferFormatting.size(transfer.bytesTransferred)) / \(TransferFormatting.size(transfer.bytesTotal))"
        )
        .font(.footnote.monospacedDigit())
        .foregroundStyle(Color(braveSystemName: .textSecondary))
      } else {
        ProgressView()
      }
      Spacer()
    }
    .padding(24)
  }

  private func finished(_ state: TransferState) -> some View {
    let name = model.snapshot.transfer?.deviceName ?? ""
    let text: String
    let symbol: String
    switch state {
    case .completed:
      text = String.localizedStringWithFormat(Strings.BraveTransfer.shareCompleted, name)
      symbol = "checkmark.circle"
    case .declined:
      text = String.localizedStringWithFormat(Strings.BraveTransfer.shareDeclined, name)
      symbol = "xmark.circle"
    case .expired:
      text = String.localizedStringWithFormat(Strings.BraveTransfer.shareExpired, name)
      symbol = "clock"
    case .cancelled:
      text = Strings.BraveTransfer.shareCancelled
      symbol = "xmark.circle"
    case .failed, .waitingForResponse, .inProgress:
      text = Strings.BraveTransfer.shareFailed
      symbol = "exclamationmark.triangle"
    }
    return message(symbol: symbol, text: text)
  }

  private func handoff(_ reason: HandoffPolicy.Reason) -> some View {
    let text: String
    switch reason {
    case .tooLarge: text = Strings.BraveTransfer.handoffTooLarge
    case .noDevice: text = Strings.BraveTransfer.handoffNoDevice
    case .localNetworkUnavailable: text = Strings.BraveTransfer.handoffLocalNetwork
    case .notSetUp: text = Strings.BraveTransfer.handoffNotSetUp
    case .userRequested: text = Strings.BraveTransfer.sharePreparing
    }
    return VStack(spacing: 16) {
      Spacer()
      Image(systemName: "arrow.up.forward.app")
        .font(.system(size: 44))
        .foregroundStyle(Color(braveSystemName: .iconInteractive))
        .accessibilityHidden(true)
      Text(text)
        .font(.headline)
        .multilineTextAlignment(.center)
        .foregroundStyle(Color(braveSystemName: .textPrimary))
      Spacer()
      Button {
        model.openApp()
      } label: {
        HStack {
          if model.isHandingOff {
            ProgressView()
          }
          Text(Strings.BraveTransfer.handoffOpenBrave)
        }
        .frame(maxWidth: .infinity)
      }
      .buttonStyle(.filled)
      .controlSize(.large)
      .disabled(model.isHandingOff)
    }
    .padding(24)
  }

  private var continueInBraveButton: some View {
    Button {
      model.continueInApp()
    } label: {
      Text(Strings.BraveTransfer.shareContinueInBrave)
        .frame(maxWidth: .infinity)
    }
    .buttonStyle(.outline)
    .controlSize(.large)
  }

  private func progressMessage(_ text: String) -> some View {
    VStack(spacing: 12) {
      ProgressView()
      Text(text)
        .foregroundStyle(Color(braveSystemName: .textSecondary))
    }
  }

  private func message(symbol: String, text: String) -> some View {
    TransferShareMessage(symbol: symbol, text: text)
  }
}

/// Shown instead of `TransferShareView` when the engine cannot start (no
/// app group container or no static key yet).
public struct TransferShareUnavailableView: View {
  var onClose: () -> Void

  public init(onClose: @escaping () -> Void) {
    self.onClose = onClose
  }

  public var body: some View {
    NavigationStack {
      TransferShareMessage(
        symbol: "exclamationmark.triangle",
        text: Strings.BraveTransfer.shareUnavailable
      )
      .frame(maxWidth: .infinity, maxHeight: .infinity)
      .background(Color(braveSystemName: .pageBackground))
      .navigationTitle(Strings.BraveTransfer.shareTitle)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItem(placement: .cancellationAction) {
          Button(Strings.BraveTransfer.shareCancel, action: onClose)
        }
      }
    }
  }
}

private struct TransferShareMessage: View {
  var symbol: String
  var text: String

  var body: some View {
    VStack(spacing: 16) {
      Image(systemName: symbol)
        .font(.system(size: 44))
        .foregroundStyle(Color(braveSystemName: .iconDefault))
        .accessibilityHidden(true)
      Text(text)
        .font(.headline)
        .multilineTextAlignment(.center)
        .foregroundStyle(Color(braveSystemName: .textPrimary))
    }
    .padding(24)
  }
}
