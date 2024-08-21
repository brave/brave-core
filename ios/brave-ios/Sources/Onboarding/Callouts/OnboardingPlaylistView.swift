// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import Foundation
import Lottie
import Strings
import SwiftUI

public class OnboardingPlaylistModel: ObservableObject {
  public enum Step: Equatable {
    case initial
    case details
    case tryItOut
    case completed(folderName: String)

    // CasePaths would have made this unneccessary...
    fileprivate var accessibilityStep: AccessibilityStep {
      switch self {
      case .initial: return .initial
      case .details: return .details
      case .tryItOut: return .tryItOut
      case .completed: return .completed
      }
    }
  }

  fileprivate enum AccessibilityStep: Hashable {
    case initial
    case details
    case tryItOut
    case completed
  }

  public var step: Step {
    get { _step }
    set {
      withAnimation(.spring(dampingFraction: 0.9)) {
        _step = newValue
      }
    }
  }

  @Published private var _step: Step

  public var onboardingCompleted: (() -> Void)?

  public init(initialStep: Step = .initial) {
    self._step = initialStep
  }
}

public struct OnboardingPlaylistView: View {
  @ObservedObject var model: OnboardingPlaylistModel

  public init(model: OnboardingPlaylistModel) {
    self.model = model
  }

  @State private var introAnimationCompleted: Bool = false

  @AccessibilityFocusState private var accessibilityFocusStep:
    OnboardingPlaylistModel.AccessibilityStep?

  @Environment(\.dismiss) private var dismiss

  private func icon(named: String, x: CGFloat, y: CGFloat, size: CGFloat = 18) -> some View {
    Image(braveSystemName: named)
      .font(.system(size: size))
      .alignmentGuide(
        HorizontalAlignment.center,
        computeValue: { d in
          d[HorizontalAlignment.center] - (x * (model.step == .initial ? 1 : 1.5))
        }
      )
      .alignmentGuide(
        VerticalAlignment.center,
        computeValue: { d in
          d[VerticalAlignment.center] - y
        }
      )
      .rotationEffect(.degrees(introAnimationCompleted ? 0 : 90))
      .scaleEffect(introAnimationCompleted ? 1 : 0.5)
      .opacity(introAnimationCompleted ? 1 : 0.0)
  }

  private func shape<S: Shape>(_ shape: S, x: CGFloat, y: CGFloat, size: CGFloat) -> some View {
    shape
      .frame(width: size, height: size)
      .alignmentGuide(
        HorizontalAlignment.center,
        computeValue: { d in
          d[HorizontalAlignment.center] - (x * (model.step == .initial ? 1 : 1.5))
        }
      )
      .alignmentGuide(
        VerticalAlignment.center,
        computeValue: { d in
          d[VerticalAlignment.center] - y
        }
      )
      .opacity(introAnimationCompleted ? 1 : 0.0)
      .scaleEffect(introAnimationCompleted ? 1 : 0.5)
      .offset(x: introAnimationCompleted ? 0 : -x, y: introAnimationCompleted ? 0 : -y)
  }

  private struct PlusShape: Shape {
    var lineWidth: CGFloat = 2

    func path(in rect: CGRect) -> Path {
      Path { p in
        p.addRoundedRect(
          in: CGRect(x: (rect.width - lineWidth) / 2, y: 0, width: lineWidth, height: rect.height),
          cornerSize: CGSize(width: 1, height: 1),
          style: .continuous
        )
        p.addRoundedRect(
          in: CGRect(x: 0, y: (rect.height - lineWidth) / 2, width: rect.width, height: lineWidth),
          cornerSize: CGSize(width: 1, height: 1),
          style: .continuous
        )
      }
    }
  }

  private var headerView: some View {
    ZStack {
      Image("playlist-audio-wave", bundle: .module)
        .resizable()
        .frame(height: 97)
      Group {
        icon(named: "leo.search.movie", x: 0, y: 45)
          .foregroundColor(Color(uiColor: .init(rgb: 0xD660C3)))
        icon(named: "leo.search.video", x: 80, y: 0)
          .foregroundColor(Color(uiColor: .init(rgb: 0xE2A500)))
        icon(named: "leo.podcast", x: -75, y: 20)
          .foregroundColor(Color(uiColor: .init(rgb: 0xAAA8F7)))
        icon(named: "leo.media.player", x: -60, y: -50)
          .foregroundColor(Color(uiColor: .init(rgb: 0xFC9378)))
        icon(named: "leo.music.tones", x: 50, y: -40)
          .foregroundColor(Color(uiColor: .init(rgb: 0xF092C5)))
      }
      Group {
        shape(Ellipse(), x: 90, y: -40, size: 10)
          .foregroundColor(Color(uiColor: .init(rgb: 0x5F5CF1)))
        shape(Ellipse(), x: 110, y: 10, size: 7)
          .foregroundColor(Color(uiColor: .init(rgb: 0x423EEE)))
        shape(Ellipse(), x: 50, y: 20, size: 6)
          .foregroundColor(Color(uiColor: .init(rgb: 0xFC9378)))
        shape(Ellipse(), x: -50, y: 40, size: 8)
          .foregroundColor(Color(uiColor: .init(rgb: 0x423EEE)))
      }
      Group {
        shape(PlusShape(), x: -100, y: -15, size: 12)
          .foregroundColor(Color(uiColor: .init(rgb: 0xFC9378)))
        shape(PlusShape(), x: -15, y: -45, size: 12)
          .foregroundColor(Color(uiColor: .init(rgb: 0x5F5CF1)))
        shape(PlusShape(), x: 90, y: 35, size: 12)
          .foregroundColor(Color(uiColor: .init(rgb: 0x5F5CF1)))
      }
      Image(sharedName: "leo.playlist.bold.add")
        .resizable()
        .aspectRatio(contentMode: .fit)
        .frame(width: 32, height: 32)
        .padding(8)
        .background(Color.white.clipShape(RoundedRectangle(cornerRadius: 12, style: .continuous)))
        .scaleEffect(introAnimationCompleted ? 1 : 0.5)
        .opacity(introAnimationCompleted ? 1 : 0.0)
    }
    .accessibilityHidden(true)
  }

  private var includesHeader: Bool {
    switch model.step {
    case .initial, .details:
      return true
    case .tryItOut, .completed:
      return false
    }
  }

  public var body: some View {
    ScrollView(.vertical) {
      VStack(spacing: 24) {
        if includesHeader {
          headerView
            .padding(.top, 24)
        }
        switch model.step {
        case .initial:
          InitialStepView(advance: {
            model.step = .details
          })
          .padding(.horizontal)
          .opacity(introAnimationCompleted ? 1 : 0.0)
          .transition(
            .asymmetric(insertion: .identity, removal: .opacity.animation(.linear(duration: 0.1)))
          )
          .accessibilityFocused($accessibilityFocusStep, equals: .initial)
        case .details:
          DetailsStepView(
            advance: {
              model.step = .tryItOut
            },
            dismiss: {
              dismiss()
            }
          )
          .transition(
            .asymmetric(
              insertion: .opacity.animation(.default.delay(0.1)),
              removal: .opacity.animation(.default)
            )
          )
          .accessibilityFocused($accessibilityFocusStep, equals: .details)
        case .tryItOut:
          TryItOutStepView()
            .accessibilitySortPriority(2)
            .transition(
              .asymmetric(
                insertion: .opacity.animation(.default.delay(0.1)),
                removal: .opacity.animation(.default)
              )
            )
            .accessibilityFocused($accessibilityFocusStep, equals: .tryItOut)
        case .completed(let folderName):
          CompletedStepView(folderName: folderName, onOpenPlaylist: model.onboardingCompleted)
            .transition(
              .asymmetric(
                insertion: .opacity.animation(.default.delay(0.1)),
                removal: .opacity.animation(.default)
              )
            )
            .accessibilityFocused($accessibilityFocusStep, equals: .completed)
        }
      }
    }
    .onChange(
      of: model.step,
      perform: { newValue in
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
          accessibilityFocusStep = newValue.accessibilityStep
        }
      }
    )
    .accessibilitySortPriority(2)
    .overlay(alignment: .bottom) {
      if model.step == .details {
        DetailsStepView(
          advance: {
            model.step = .tryItOut
          },
          dismiss: {
            dismiss()
          }
        )
        .expandedViewButtons
        .foregroundStyle(.white)
        .accessibilitySortPriority(1)
      }
    }
    .frame(
      minWidth: model.step == .initial ? nil : 256,
      idealWidth: model.step == .initial ? 256 : nil,
      maxWidth: BraveUX.baseDimensionValue
    )
    .task {
      do {
        try await Task.sleep(nanoseconds: NSEC_PER_MSEC * 250)
        withAnimation(.spring(dampingFraction: 0.7)) {
          introAnimationCompleted = true
        }
      } catch {}
    }
    .foregroundStyle(.white)
    .multilineTextAlignment(.center)
    .osAvailabilityModifiers { content in
      #if compiler(>=5.8)
      if #available(iOS 16.4, *) {
        content
          .scrollBounceBehavior(.basedOnSize)
      } else {
        content
          .introspectScrollView { scrollView in
            scrollView.alwaysBounceVertical = false
          }
      }
      #else
      content
        .introspectScrollView { scrollView in
          scrollView.alwaysBounceVertical = false
        }
      #endif
    }
  }
}

extension OnboardingPlaylistView: PopoverContentComponent {
  public var popoverBackgroundColor: UIColor {
    UIColor(rgb: 0x3835CA)
  }
}

extension OnboardingPlaylistView {
  struct InitialStepView: View {
    var advance: () -> Void
    @Environment(\.dismiss) private var dismiss

    var body: some View {
      VStack(spacing: 24) {
        VStack(spacing: 12) {
          Text(Strings.PlaylistOnboarding.introducingBravePlaylist)
            .font(.subheadline.weight(.semibold))
            .accessibilityAddTraits(.isHeader)
          Text(Strings.PlaylistOnboarding.playlistOnboardingDescription)
            .font(.footnote)
        }
        VStack(spacing: 16) {
          Button {
            advance()
          } label: {
            Text(Strings.PlaylistOnboarding.advanceInitialOnboardingButtonTitle)
              .frame(maxWidth: .infinity)
          }
          .buttonStyle(PlaylistButtonStyle())
          Button {
            dismiss()
          } label: {
            Text(Strings.PlaylistOnboarding.dismissOnboardingButtonTitle)
              .font(.footnote.weight(.semibold))
              .frame(maxWidth: .infinity)
          }
        }
      }
      .padding(.bottom, 24)
    }
  }

  struct DetailsStepView: View {
    var advance: () -> Void
    var dismiss: () -> Void

    var body: some View {
      VStack(spacing: 24) {
        Text(Strings.PlaylistOnboarding.introducingBravePlaylist)
          .font(.title2.weight(.semibold))
          .padding(.horizontal)
          .accessibilityAddTraits(.isHeader)
        VStack(spacing: 24) {
          VStack(spacing: 24) {
            bulletPoint(
              braveSystemName: "leo.cloud.download",
              title: Strings.PlaylistOnboarding.playlistInfoFeaturePointTitleOne,
              subtitle: Strings.PlaylistOnboarding.playlistInfoFeaturePointSubtitleOne
            )
            bulletPoint(
              braveSystemName: "leo.product.playlist-add",
              title: Strings.PlaylistOnboarding.playlistInfoFeaturePointTitleTwo,
              subtitle: Strings.PlaylistOnboarding.playlistInfoFeaturePointSubtitleTwo
            )
            bulletPoint(
              braveSystemName: "leo.play.circle",
              title: Strings.PlaylistOnboarding.playlistInfoFeaturePointTitleThree,
              subtitle: Strings.PlaylistOnboarding.playlistInfoFeaturePointSubtitleThree
            )
          }
          .padding(.horizontal, 32)
          expandedViewButtons  // For layout purposes
            .hidden()
        }
      }
    }

    var expandedViewButtons: some View {
      VStack(spacing: 16) {
        Button {
          advance()
        } label: {
          Text(Strings.PlaylistOnboarding.advanceStep2OnboardingButtonTitle)
            .font(.subheadline.weight(.semibold))
            .frame(maxWidth: .infinity)
        }
        .buttonStyle(PlaylistButtonStyle())
        Button {
          dismiss()
        } label: {
          Text(Strings.PlaylistOnboarding.dismissStep2OnboardingButtonTitle)
            .font(.footnote.weight(.semibold))
            .frame(maxWidth: .infinity)
        }
      }
      .padding(24)
      .background(Color(UIColor(rgb: 0x322FB4)))
    }

    @ScaledMetric private var bulletPointIconSize: CGFloat = 32

    private func bulletPoint(braveSystemName: String, title: String, subtitle: String) -> some View
    {
      Label {
        VStack(alignment: .leading, spacing: 4) {
          Text(title)
            .font(.subheadline.weight(.medium))
          Text(subtitle)
            .font(.footnote)
        }
        .multilineTextAlignment(.leading)
        .frame(maxWidth: .infinity, alignment: .leading)
      } icon: {
        Image(braveSystemName: braveSystemName)
          .foregroundStyle(
            LinearGradient(
              braveGradient: .init(
                stops: [
                  .init(color: UIColor(rgb: 0xFA7250), position: 0.0),
                  .init(color: UIColor(rgb: 0xFF1893), position: 0.43),
                  .init(color: UIColor(rgb: 0xA78AFF), position: 1.0),
                ],
                angle: .figmaDegrees(314.42)
              )
            )
          )
          .frame(width: bulletPointIconSize, height: bulletPointIconSize)
          .background(Color.white.clipShape(Ellipse()))
      }
      .labelStyle(BulletLabelStyle())
    }

    private struct BulletLabelStyle: LabelStyle {
      func makeBody(configuration: Configuration) -> some View {
        HStack(spacing: 16) {
          configuration.icon
          configuration.title
        }
      }
    }
  }

  struct TryItOutStepView: View {
    @ScaledMetric private var tryItOutIconSize = 20
    @ScaledMetric private var tryItOutIconBaselineOffset = -6

    var textWithIconNextToPrompt: Text {
      // Since we need this copy localized correctly and we want the icon always to live next to
      // the "Add to Playlist" (atp for short) text, we need to pull things apart
      let copy = String.localizedStringWithFormat(
        Strings.PlaylistOnboarding.userActionPrompt,
        Strings.PlaylistOnboarding.userActionPromptAddToPlaylist
      )
      let atpRange = copy.range(of: Strings.PlaylistOnboarding.userActionPromptAddToPlaylist)!

      // "Tap"
      let copyToATP = String(copy[copy.startIndex..<atpRange.lowerBound])
        .trimmingCharacters(in: .whitespaces)
      // "Add to Playlist"
      let addToPlaylist = Strings.PlaylistOnboarding.userActionPromptAddToPlaylist
      // "to add your first item"
      let copyFromATPEnd = String(copy[atpRange.upperBound..<copy.endIndex])
        .trimmingCharacters(in: .whitespaces)
      // The icon, which is a PDF since it is multicoloured and has special alpha, needs to scale with
      // the text, so we need to create a UIImage version of it, scale that, then adjust the baseline offset
      let addIcon = UIImage(sharedNamed: "leo.playlist.bold.add")!
      let icon = Image(
        uiImage: addIcon.createScaled(CGSize(width: tryItOutIconSize, height: tryItOutIconSize))
          ?? addIcon
      )
      let iconText = Text(icon).baselineOffset(tryItOutIconBaselineOffset)

      return Text("\(copyToATP) '\(addToPlaylist)' \(iconText) \(copyFromATPEnd)")
    }

    var body: some View {
      textWithIconNextToPrompt
        .multilineTextAlignment(.center)
        .foregroundColor(.white)
        .font(.footnote)
        .padding(.horizontal, 16)
        .padding(.vertical, 12)
    }
  }

  struct CompletedStepView: View {
    var folderName: String
    var onOpenPlaylist: (() -> Void)?

    @Environment(\.dismiss) var dismiss

    var body: some View {
      VStack(spacing: 24) {
        VStack {
          Text(Strings.PlaylistOnboarding.confirmationTitle)
            .font(.title2.weight(.medium))
          Text(
            String.localizedStringWithFormat(
              Strings.PlaylistOnboarding.confirmationSubtitle,
              folderName
            )
          )
          .font(.callout)
        }
        .padding(.vertical)
        .padding()
        .frame(maxWidth: .infinity)
        .background {
          LottieView(
            animation: .named(
              "playlist-confetti",
              bundle: .module
            )
          )
          .playing()
          .resizable()
          .aspectRatio(contentMode: .fill)
        }
        .clipShape(Rectangle())
        .background(Color(uiColor: .init(rgb: 0x423EEE)))
        VStack(spacing: 24) {
          Text(Strings.PlaylistOnboarding.confirmationPrompt)
            .font(.callout)
          Button {
            onOpenPlaylist?()
          } label: {
            Text(Strings.PlaylistOnboarding.confirmationActionButtonTitle)
              .font(.subheadline.weight(.semibold))
              .frame(maxWidth: .infinity)
              .padding(.vertical, 4)
          }
          .buttonStyle(PlaylistButtonStyle())
          Button {
            dismiss()
          } label: {
            Text(Strings.PlaylistOnboarding.dismissStep4OnboardingButtonTitle)
              .font(.footnote.weight(.semibold))
              .frame(maxWidth: .infinity)
          }
        }
        .padding([.horizontal, .bottom], 24)
      }
    }
  }
}

// A Variant of BraveFilledButtonStyle
private struct PlaylistButtonStyle: ButtonStyle {
  private let clipShape = RoundedRectangle(cornerRadius: 48, style: .continuous)

  public func makeBody(configuration: Configuration) -> some View {
    configuration.label
      .opacity(configuration.isPressed ? 0.7 : 1.0)
      .font(.footnote.weight(.semibold))
      .foregroundColor(.white)
      .padding(8)
      .background(
        Color(.white).opacity(0.25).opacity(configuration.isPressed ? 0.7 : 1.0)
      )
      .clipShape(clipShape)
      .contentShape(clipShape)
      .hoverEffect()
  }
}

#if DEBUG
struct OnboardingPlaylistView_PreviewProvider: PreviewProvider {
  static var previews: some View {
    let model = OnboardingPlaylistModel(initialStep: .completed(folderName: "Play Later"))
    OnboardingPlaylistView(
      model: model
    )
    .clipShape(RoundedRectangle(cornerRadius: 8, style: .continuous))
    .background(
      Color(UIColor(rgb: 0x3835CA))
        .clipShape(RoundedRectangle(cornerRadius: 8, style: .continuous))
        .shadow(radius: 3, y: 2)
    )
    .padding()
    .background {
      Color.clear
        .contentShape(Rectangle())
        .onTapGesture {
          if case .completed = model.step {
            model.step = .initial
          } else {
            model.step = .completed(folderName: "Play Later")
          }
        }
    }
    .fixedSize(horizontal: false, vertical: true)
    .previewLayout(.sizeThatFits)
  }
}
#endif
