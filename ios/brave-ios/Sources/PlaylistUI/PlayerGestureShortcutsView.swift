// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

/// An overlay on top of the PlayerView that allows the user to quickly activate certain shortcuts
/// by performing gestures on it such as tap to play/pause, double-tap to seek in a direction
struct PlayerGestureShortcutsView: View {
  var model: PlayerModel

  enum SeekDirection {
    case backwards
    case forwards
  }

  @State private var autoHideSeekNoticeTask: Task<Void, Error>?
  @State private var seekDirectionNoticeVisible: SeekDirection?
  @State private var seekIntervalPower: CGFloat = 0

  private func hideSeekNoticeAfterDelay() {
    autoHideSeekNoticeTask = Task {
      try await Task.sleep(for: .seconds(1))
      withAnimation(.linear(duration: 0.1)) {
        seekDirectionNoticeVisible = nil
      }
      seekIntervalPower = 0
    }
  }

  var body: some View {
    GeometryReader { proxy in
      Color.clear
        .contentShape(.rect.inset(by: 20))
        .onTapGesture(count: 2) { point in
          autoHideSeekNoticeTask?.cancel()
          var seekAdjustment = model.seekInterval * pow(2.0, seekIntervalPower)
          if point.x < proxy.size.width / 2 {
            seekDirectionNoticeVisible = .backwards
            seekAdjustment *= -1
          } else {
            seekDirectionNoticeVisible = .forwards
          }
          let seekTarget = model.currentTime + seekAdjustment
          seekIntervalPower += 1
          Task {
            await model.seek(to: seekTarget, accurately: false)
            hideSeekNoticeAfterDelay()
          }
        }
        .onTapGesture {
          model.isPlaying.toggle()
        }
        .onChange(of: seekDirectionNoticeVisible) { _ in
          // Reset if the notice changes
          seekIntervalPower = 0
        }
        .overlay(alignment: seekDirectionNoticeVisible == .backwards ? .leading : .trailing) {
          if let seekDirectionNoticeVisible {
            VStack(
              alignment: seekDirectionNoticeVisible == .backwards ? .leading : .trailing,
              spacing: 4
            ) {
              Image(
                braveSystemName: seekDirectionNoticeVisible == .backwards
                  ? "leo.rewind.filled" : "leo.forward.filled"
              )
              Text(
                Duration.seconds(model.seekInterval * pow(2.0, seekIntervalPower)),
                format: .units(allowed: [.seconds], width: .abbreviated)
              )
            }
            .font(.footnote.weight(.semibold))
            .padding()
            .frame(
              width: proxy.size.width / 2,
              height: proxy.size.height,
              alignment: seekDirectionNoticeVisible == .backwards ? .leading : .trailing
            )
            .background {
              LinearGradient(
                colors: [.black.opacity(0.6), .black.opacity(0)],
                startPoint: seekDirectionNoticeVisible == .backwards ? .leading : .trailing,
                endPoint: seekDirectionNoticeVisible == .backwards ? .trailing : .leading
              )
            }
            .allowsHitTesting(false)
          }
        }
    }
  }
}
