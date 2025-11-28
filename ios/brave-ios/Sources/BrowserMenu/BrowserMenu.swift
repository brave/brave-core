// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveUI
import DesignSystem
import GuardianConnect
import Preferences
import Strings
import SwiftUI

public struct BrowserMenu: View {
  @ObservedObject var model: BrowserMenuModel
  var handlePresentation: (BrowserMenuPresentation) -> Void
  var onShowAllActions: (() -> Void)?

  @ObservedObject private var numberOfQuickActions = Preferences.BrowserMenu.numberOfQuickActions
  @State private var isEditMenuPresented = false

  @State private var activeActionHandlers: [Action.ID: Task<Void, Never>] = [:]
  @State private var isAdditionalActionsVisible: Bool = false

  @Environment(\.dynamicTypeSize) private var dynamicTypeSize

  private func handleAction(_ action: Binding<Action>) {
    let id = action.wrappedValue.id
    if activeActionHandlers[id] != nil {
      // The action is in progress
      return
    }
    let actionTask = Task {
      defer {
        activeActionHandlers[id] = nil
      }
      let result = await action.wrappedValue.handler(action.wrappedValue)
      switch result {
      case .updateAction(let replacement):
        withAnimation(.snappy) {
          action.wrappedValue = replacement
        }
        break
      case .none:
        break
      }
    }
    activeActionHandlers[id] = actionTask
  }

  private var quickActions: Binding<[Action]> {
    Binding(
      get: { Array(model.visibleActions.prefix(numberOfQuickActions.value)) },
      set: {
        model.visibleActions.replaceSubrange(
          0..<min(numberOfQuickActions.value, model.visibleActions.count),
          with: $0
        )
      }
    )
  }

  private var listedActions: Binding<[Action]> {
    Binding(
      get: {
        if model.visibleActions.count < numberOfQuickActions.value {
          return []
        }
        return Array(model.visibleActions[numberOfQuickActions.value...])
      },
      set: {
        if model.visibleActions.count > numberOfQuickActions.value {
          model.visibleActions.replaceSubrange(numberOfQuickActions.value..., with: $0)
        }
      }
    )
  }

  public var body: some View {
    ScrollView {
      VStack(spacing: 16) {
        VStack(alignment: .leading) {
          HStack {
            Text(Strings.BrowserMenu.myActions)
              .font(.caption.weight(.semibold))
              .textCase(.uppercase)
            Spacer()
            Button {
              isEditMenuPresented = true
            } label: {
              Text(Strings.BrowserMenu.editButtonTitle)
                .font(.caption.weight(.semibold))
                .padding(.vertical, 4)
                .padding(.horizontal, 8)
            }
          }
          .foregroundStyle(Color(braveSystemName: .textTertiary))
          QuickActionsView(actions: quickActions) { $action in
            handleAction($action)
          }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .leading)
        ActionsList(
          actions: listedActions,
          additionalActions: $model.hiddenActions,
          isAdditionalActionsVisible: $isAdditionalActionsVisible,
          handler: { $action in
            handleAction($action)
          }
        )
        if case .connected(let region) = model.vpnStatus {
          Button {
            handlePresentation(.vpnRegionPicker)
          } label: {
            Label {
              HStack {
                Text(Strings.BrowserMenu.vpnButtonTitle)
                Spacer()
                Text(region.flag)
                Text(region.displayName)
                if model.vpnStatus.shouldShowSmartProxyIndicator {
                  Image(braveSystemName: "leo.smart.proxy-routing")
                    .resizable()
                    .renderingMode(.template)
                    .aspectRatio(contentMode: .fit)
                    .foregroundColor(Color(braveSystemName: .iconDefault))
                    .frame(width: 14, height: 14)
                    .padding(4)
                    .background(
                      Color(braveSystemName: .containerHighlight),
                      in: .rect(cornerRadius: 4, style: .continuous)
                    )
                }
              }
            } icon: {
              Image(braveSystemName: "leo.product.vpn")
            }
          }
          .buttonStyle(MenuRowButtonStyle())
          .background(Color(braveSystemName: .schemesSurfaceBright))
          .clipShape(.rect(cornerRadius: 14, style: .continuous))
          .transition(.blurReplace())
        }
        Button {
          handlePresentation(.settings)
        } label: {
          Label(Strings.BrowserMenu.allSettingsButtonTitle, braveSystemImage: "leo.settings")
        }
        .buttonStyle(MenuRowButtonStyle())
        .background(Color(braveSystemName: .schemesSurfaceBright))
        .clipShape(.rect(cornerRadius: 14, style: .continuous))
      }
      .padding()
    }
    .enableButtonScrollViewDragWorkaround()
    .background(Color(braveSystemName: .iosBrowserChromeBackgroundIos))
    .dynamicTypeSize(DynamicTypeSize.xSmall..<DynamicTypeSize.accessibility3)
    .osAvailabilityModifiers { content in
      // "Designed for iPad" VisionOS - FB16385402
      //
      // There is a bug unfortunately on "Designed for iPad" VisionOS apps in SwiftUI where a
      // sheet presented on top of a popover will dismiss the popover before presenting the sheet
      // (thus then dismissing the sheet since the underlying State/View hierarchy will dealloc)
      //
      // As a workaround, VisionOS will present a popover for the edit menu. The fixed frame
      // is fine because iPad apps running on VisionOS have a fixed size (in both portrait and
      // landscape forced orientations)
      if ProcessInfo.processInfo.isiOSAppOnVisionOS {
        content
          .popover(isPresented: $isEditMenuPresented) {
            CustomizeMenuView(model: model)
              .frame(width: 600, height: 600)
          }
      } else {
        content
          .sheet(isPresented: $isEditMenuPresented) {
            CustomizeMenuView(model: model)
          }
      }
    }
    .onChange(of: isAdditionalActionsVisible) { _, newValue in
      if newValue {
        onShowAllActions?()
      }
    }
  }
}

private struct QuickActionsView: View {
  @Binding var actions: [Action]
  var handler: (Binding<Action>) -> Void

  var body: some View {
    LazyVGrid(
      columns: .init(repeating: .init(.flexible(), spacing: 12, alignment: .top), count: 4),
      spacing: 12
    ) {
      ForEach($actions) { $action in
        Button {
          handler($action)
        } label: {
          Label {
            Text(action.title)
          } icon: {
            Image(braveSystemName: action.image)
          }
        }
        .buttonStyle(QuickActionButtonStyle(state: action.state, traits: action.traits))
        .disabled(action.attributes.contains(.disabled))
        .transition(.blurReplace())
        .id(action)
      }
    }
  }

  private struct QuickActionButtonStyle: ButtonStyle {
    var state: Bool?
    var traits: Action.Traits

    func makeBody(configuration: Configuration) -> some View {
      configuration.label
        .labelStyle(_LabelStyle(isPressed: configuration.isPressed, state: state, traits: traits))
    }

    private struct _LabelStyle: LabelStyle {
      @Environment(\.isEnabled) private var isEnabled
      @ScaledMetric private var iconFrameSize = 22
      @ScaledMetric private var iconFontSize = 18
      @ScaledMetric private var badgeRadius = 8
      var isPressed: Bool
      var state: Bool?
      var traits: Action.Traits

      func makeBody(configuration: Configuration) -> some View {
        VStack(spacing: 8) {
          configuration.icon
            .frame(width: iconFrameSize, height: iconFrameSize)
            .padding(.vertical, 12)
            .font(.system(size: iconFontSize))
            .foregroundStyle(
              state == true
                ? Color(braveSystemName: .schemesOnPrimary)
                : isEnabled
                  ? Color(braveSystemName: .iconDefault)
                  : Color(braveSystemName: .iconDisabled)
            )
            .frame(maxWidth: .infinity)
            .background {
              Color(braveSystemName: state == true ? .schemesPrimary : .schemesSurfaceBright)
                .overlay(
                  Color(
                    braveSystemName: state == true
                      ? .schemesPrimary : .iosBrowserContainerHighlightIos
                  ).opacity(isPressed ? 1 : 0)
                )
                .opacity(state == true && isPressed ? 0.5 : 1)
                .clipShape(.rect(cornerRadius: 14, style: .continuous))
                .hoverEffect()
            }
            .overlay(alignment: .topTrailing) {
              if let badgeColor = traits.badgeColor {
                Circle()
                  .fill(Color(uiColor: badgeColor))
                  .frame(width: badgeRadius, height: badgeRadius)
                  .overlay {
                    Circle()
                      .stroke(Color(braveSystemName: .iosBrowserChromeBackgroundIos), lineWidth: 1)
                  }
                  .allowsHitTesting(false)
              }
            }
          configuration.title
            .font(.caption2)
            .lineLimit(2)
            .foregroundStyle(
              isEnabled
                ? Color(braveSystemName: .textPrimary)
                : Color(braveSystemName: .textDisabled)
            )
            .multilineTextAlignment(.center)
        }
        .opacity(isEnabled ? 1 : 0.7)
        .contentShape(.rect)
      }
    }
  }
}

private struct ActionButton: View {
  @Binding var action: Action
  var handler: (Binding<Action>) -> Void
  var badgeRadius: CGFloat

  var body: some View {
    Button {
      handler($action)
    } label: {
      HStack {
        Label {
          Text(action.title)
        } icon: {
          Image(braveSystemName: action.image)
        }
        Spacer()
        if let badgeColor = action.traits.badgeColor {
          Circle()
            .fill(Color(uiColor: badgeColor))
            .frame(width: badgeRadius, height: badgeRadius)
            .allowsHitTesting(false)
        }
        if let state = action.state {
          Toggle("", isOn: .constant(state))
            .tint(Color(braveSystemName: .primary50))
            .labelsHidden()
            .allowsHitTesting(false)
        }
      }
    }
    .buttonStyle(MenuRowButtonStyle())
    .disabled(action.attributes.contains(.disabled))
    .transition(.blurReplace())
    .id(action)
  }
}

private struct ActionsList: View {
  @Binding var actions: [Action]
  @Binding var additionalActions: [Action]
  @Binding var isAdditionalActionsVisible: Bool
  var handler: (Binding<Action>) -> Void

  init(
    actions: Binding<[Action]>,
    additionalActions: Binding<[Action]>,
    isAdditionalActionsVisible: Binding<Bool>,
    handler: @escaping (Binding<Action>) -> Void
  ) {
    self._actions = actions
    self._additionalActions = additionalActions
    self._isAdditionalActionsVisible = isAdditionalActionsVisible
    self.handler = handler
  }

  init(
    action: Action,
    handler: @escaping (Action) -> Void
  ) {
    self._actions = .constant([action])
    self._additionalActions = .constant([])
    self._isAdditionalActionsVisible = .constant(false)
    self.handler = { action in
      handler(action.wrappedValue)
    }
  }

  @Environment(\.pixelLength) private var pixelLength
  @ScaledMetric private var badgeRadius = 8

  @ViewBuilder private func labelForAction(_ action: Action) -> some View {
    HStack {
      Label {
        Text(action.title)
      } icon: {
        Image(braveSystemName: action.image)
      }
      Spacer()
      if let badgeColor = action.traits.badgeColor {
        Circle()
          .fill(Color(uiColor: badgeColor))
          .frame(width: badgeRadius, height: badgeRadius)
          .allowsHitTesting(false)
      }
      if let state = action.state {
        Toggle("", isOn: .constant(state))
          .tint(Color(braveSystemName: .primary50))
          .labelsHidden()
          .allowsHitTesting(false)
      }
    }
  }

  var body: some View {
    VStack(alignment: .leading, spacing: 0) {
      ForEach($actions) { $action in
        ActionButton(action: $action, handler: handler, badgeRadius: badgeRadius)
        if !additionalActions.isEmpty || action.id != actions.last?.id {
          Color(braveSystemName: .materialDivider)
            .frame(height: pixelLength)
            .padding(.leading, 16)
        }
      }
      if !additionalActions.isEmpty {
        if !isAdditionalActionsVisible {
          Button {
            withAnimation(.snappy) {
              isAdditionalActionsVisible = true
            }
          } label: {
            Label(Strings.BrowserMenu.showAllButtonTitle, braveSystemImage: "leo.more.horizontal")
          }
          .buttonStyle(MenuRowButtonStyle())
        }
        if isAdditionalActionsVisible {
          ForEach($additionalActions) { $action in
            ActionButton(action: $action, handler: handler, badgeRadius: badgeRadius)
            if action.id != additionalActions.last?.id {
              Color(braveSystemName: .materialDivider)
                .frame(height: pixelLength)
                .padding(.leading, 16)
            }
          }
        }
      }
    }
    .background(Color(braveSystemName: .schemesSurfaceBright))
    .clipShape(.rect(cornerRadius: 14, style: .continuous))
  }
}

private struct MenuRowButtonStyle: ButtonStyle {
  func makeBody(configuration: Configuration) -> some View {
    configuration.label
      .frame(minHeight: 46)
      .padding(.horizontal, 16)
      .frame(maxWidth: .infinity, minHeight: 44, alignment: .leading)
      .contentShape(.rect)
      .hoverEffect()
      .background(
        Color(braveSystemName: .iosBrowserContainerHighlightIos)
          .opacity(configuration.isPressed ? 1 : 0)
      )
      .labelStyle(_LabelStyle())
  }

  private struct _LabelStyle: LabelStyle {
    @ScaledMetric private var iconFrameSize = 22
    @ScaledMetric private var iconFontSize = 18
    @Environment(\.isEnabled) private var isEnabled

    func makeBody(configuration: Configuration) -> some View {
      HStack(spacing: 10) {
        configuration.icon
          .frame(width: iconFrameSize, height: iconFrameSize)
          .font(.system(size: iconFontSize))
          .foregroundStyle(
            isEnabled
              ? Color(braveSystemName: .iconDefault)
              : Color(braveSystemName: .iconDisabled)
          )
        configuration.title
          .font(.body)
          .foregroundStyle(
            isEnabled
              ? Color(braveSystemName: .textPrimary)
              : Color(braveSystemName: .textDisabled)
          )
      }
      .padding(.vertical, 12)
      .opacity(isEnabled ? 1 : 0.7)
    }
  }
}

#if DEBUG
#Preview {
  BrowserMenu(model: .mock, handlePresentation: { _ in })
    .background(
      LinearGradient(
        colors: [Color.red, Color.blue, Color.purple, Color.green],
        startPoint: .topLeading,
        endPoint: .bottomTrailing
      )
    )
}
#endif
