// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Strings
import SwiftUI

enum ItemDownloadState {
  case downloading(percentComplete: Double)
  case completed
}

struct PlaylistItemView: View {
  var title: String
  var assetURL: URL?
  var pageURL: URL?
  var duration: PlayerModel.ItemDuration
  var isSelected: Bool
  var isPlaying: Bool
  var downloadState: ItemDownloadState?

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
        .background(Color(braveSystemName: .neutral20))
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
            .frame(maxWidth: .infinity, alignment: .leading)
          if isSelected {
            LeoPlayingSoundView(isAnimating: isPlaying)
              .frame(width: 16, height: 16)
              .tint(Color(braveSystemName: .primary50))
          }
        }
        HStack(alignment: .firstTextBaseline) {
          switch duration {
          case .seconds(let duration):
            Text(.seconds(duration), format: .time(pattern: .minuteSecond))
          case .indefinite:
            Text(Strings.Playlist.liveIndicator)
          case .unknown:
            EmptyView()
          }
          if let downloadState {
            switch downloadState {
            case .downloading(let percentCompleted):
              Label {
                Text(Strings.Playlist.itemDownloadStatusPreparing)
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
                Text(Strings.Playlist.accessibilityItemDownloadStatusReady)
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
  }
}

/// A custom View that displays the `leo.playing.sound` icon but is animatable & customizable
///
/// While `isAnimating` is true, the bars of the icon will continuously randomize and use the `tint`
/// foreground style. If `isAnimating` is false then bars will revert to 10% of total height and
/// greyscale.
struct LeoPlayingSoundView: View {
  var isAnimating: Bool

  @State private var barHeights: SIMD4<Double> = .init(x: 0.1, y: 0.1, z: 0.1, w: 0.1)

  private func randomizeBarHeights() {
    barHeights = .init(
      x: Double.random(in: 0.2...0.45),
      y: Double.random(in: 0.3...1),
      z: Double.random(in: 0.4...0.75),
      w: Double.random(in: 0.5...0.9)
    )
  }

  var body: some View {
    LeoPlayingSoundShape(barHeights: barHeights)
      .foregroundStyle(.tint)
      .grayscale(isAnimating ? 0 : 1)
      .animation(.linear(duration: 0.3), value: barHeights)
      .onReceive(
        Timer.publish(every: 0.3, on: .main, in: .default).autoconnect(),
        perform: { _ in
          if !isAnimating || UIApplication.shared.applicationState == .background { return }
          randomizeBarHeights()
        }
      )
      .onAppear {
        if isAnimating {
          randomizeBarHeights()
        }
      }
      .onChange(of: isAnimating) { newValue in
        if newValue {
          randomizeBarHeights()
        } else {
          barHeights = .init(
            x: 0.1,
            y: 0.1,
            z: 0.1,
            w: 0.1
          )
        }
      }
  }

  /// The underlying shape that renders the glyph with varying bar heights
  ///
  /// We use a custom shape so that we can animate the shape using `Animatable`/`animatableData`
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

    // SIMD4 only being used here as a 4-value container
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
#Preview {
  LazyVStack(spacing: 12) {
    Button {
    } label: {
      PlaylistItemView(
        title: "The Worst Product I've Ever Reviewed... For Now",
        duration: .seconds(1504),
        isSelected: true,
        isPlaying: true
      )
    }
    Button {
    } label: {
      PlaylistItemView(
        title: "1 Hour of Epic Final Fantasy Remixes",
        duration: .seconds(3081),
        isSelected: true,
        isPlaying: false,
        downloadState: .completed
      )
    }
    Button {
    } label: {
      PlaylistItemView(
        title: "Conan O'Brien Needs a Doctor While Eating Spicy Wings | Hot Ones",
        duration: .seconds(1641),
        isSelected: false,
        isPlaying: false,
        downloadState: .downloading(percentComplete: 0.33)
      )
    }
  }
  .padding()
}
#endif
