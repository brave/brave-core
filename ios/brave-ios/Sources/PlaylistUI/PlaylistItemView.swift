// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

// FIXME: Move to item view model
enum DownloadState {
  case downloading(percentComplete: Double)
  case completed
}

@available(iOS 16.0, *)
struct PlaylistItemView: View {
  var title: String
  // FIXME: We'd need to support non-video specific entries as well eventually such as webpage TTS
  // those pages won't have duration, and they'll show a simple favicon in the center of the thumbnail
  var assetURL: URL?
  var pageURL: URL?
  var duration: Duration
  var isItemPlaying: Bool
  var downloadState: DownloadState?

  @ScaledMetric private var progressViewSize = 16
  // Don't need to scale the guage line width, but it looks better when it does
  @ScaledMetric private var gaugeLineWidth = 2

  var body: some View {
    HStack(spacing: 12) {
      Color.clear
        .aspectRatio(1.333, contentMode: .fit)
        .frame(height: 90)
        .overlay {
          if let assetURL, let pageURL {
            MediaThumbnail(assetURL: assetURL, pageURL: pageURL)
          }
        }
        .background(Color(braveSystemName: .gray20))
        .clipShape(ContainerRelativeShape())
        .containerShape(RoundedRectangle(cornerRadius: 8, style: .continuous))
      VStack(alignment: .leading, spacing: 4) {
        HStack(alignment: .top) {
          Text(title)
            .multilineTextAlignment(.leading)
            .lineLimit(2)
            .fixedSize(horizontal: false, vertical: true)
            .font(.callout.weight(.semibold))
            .foregroundColor(Color(braveSystemName: .textPrimary))
          if isItemPlaying {
            LeoPlayingSoundView()
              // FIXME: Should this scale? Its just cosmetic
              .frame(width: 16, height: 16)
              .foregroundStyle(Color(braveSystemName: .primary50))
          }
        }
        HStack(alignment: .firstTextBaseline) {
          Text(duration, format: .time(pattern: .minuteSecond))
          if let downloadState {
            switch downloadState {
            case .downloading(let percentCompleted):
              Label {
                Text("Preparing")
              } icon: {
                Gauge(value: percentCompleted) {
                  EmptyView()
                }
                .gaugeStyle(.circularCapacity(lineWidth: gaugeLineWidth))
                .frame(width: progressViewSize, height: progressViewSize)
              }
              .foregroundColor(Color(braveSystemName: .primary50))
            case .completed:
              Label {
                // FIXME: Better accessibility label
                Text("Item Ready")
              } icon: {
                Image(braveSystemName: "leo.check.circle-outline")
              }
              .labelStyle(.iconOnly)
            }
          }
        }
        .font(.footnote)
        .foregroundStyle(Color(braveSystemName: .textSecondary))
      }
      .frame(maxWidth: .infinity, alignment: .leading)
    }
    .padding(.horizontal, 12)
    .padding(.vertical, 8)
  }
}

// FIXME: Replace with a TimelineView variant if possible
struct LeoPlayingSoundView: View {
  @State private var barHeights: SIMD4<Double> = .init(x: 0.45, y: 1, z: 0.6, w: 0.8)

  var body: some View {
    LeoPlayingSoundShape(barHeights: barHeights)
      .animation(.linear(duration: 0.3), value: barHeights)
      .onReceive(
        Timer.publish(every: 0.3, on: .main, in: .default).autoconnect(),
        perform: { _ in
          barHeights = .init(
            x: Double.random(in: 0.2...0.45),
            y: Double.random(in: 0.3...1),
            z: Double.random(in: 0.4...0.75),
            w: Double.random(in: 0.5...0.9)
          )
        }
      )
  }

  struct LeoPlayingSoundShape: Shape {
    var animatableData:
      AnimatablePair<AnimatablePair<CGFloat, CGFloat>, AnimatablePair<CGFloat, CGFloat>>
    {
      get { .init(.init(barHeights[0], barHeights[1]), .init(barHeights[2], barHeights[3])) }
      set {
        barHeights = [
          newValue.first.first, newValue.first.second, newValue.second.first,
          newValue.second.second,
        ]
      }
    }

    var barHeights: SIMD4<Double> = .init(x: 0.45, y: 1, z: 0.6, w: 0.8)

    func path(in rect: CGRect) -> Path {
      Path { p in
        let separatorWidth = rect.width * 0.12
        let barWidth = rect.width * 0.15
        for i in 0..<4 {
          let height = rect.height * barHeights[i]
          p.addRoundedRect(
            in: .init(
              x: (CGFloat(i) * barWidth) + (CGFloat(i) * separatorWidth),
              y: rect.height - height,
              width: barWidth,
              height: height
            ),
            cornerSize: .init(width: 2, height: 2),
            style: .continuous
          )
        }
      }
    }
  }
}

#if DEBUG
// swift-format-ignore
@available(iOS 16.0, *)
#Preview {
  LazyVStack(spacing: 0) {
    Button {
    } label: {
      PlaylistItemView(title: "The Worst Product I've Ever Reviewed... For Now", duration: .seconds(1504), isItemPlaying: true)
    }
    Button {
    } label: {
      PlaylistItemView(title: "1 Hour of Epic Final Fantasy Remixes", duration: .seconds(3081), isItemPlaying: false, downloadState: .completed)
    }
    Button {
    } label: {
      PlaylistItemView(title: "Conan O'Brien Needs a Doctor While Eating Spicy Wings | Hot Ones", duration: .seconds(1641), isItemPlaying: false, downloadState: .downloading(percentComplete: 0.33))
    }
  }
  .buttonStyle(.spring(scale: 0.9))
}
#endif
