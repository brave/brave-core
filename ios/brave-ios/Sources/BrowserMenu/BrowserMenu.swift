// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import GuardianConnect
import Strings
import SwiftUI

public struct BrowserMenu: View {
  @ObservedObject var model: BrowserMenuModel
  var handlePresentation: (BrowserMenuPresentation) -> Void

  @State private var isEditMenuPresented = false

  /// Whether or not we may be viewing on a device that doesn't have as much horizontal space
  /// available (such as when Display Zoom is enabled)
  @State private var isHorizontalSpaceRestricted: Bool = false

  @Environment(\.dynamicTypeSize) private var dynamicTypeSize

  private func handleAction(_ action: Binding<Action>) {
    Task {
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
  }

  private var numberOfQuickActions: Int {
    if model.visibleActions.count < 4 {
      return model.visibleActions.count
    }
    if dynamicTypeSize.isAccessibilitySize || isHorizontalSpaceRestricted {
      return 4
    }
    return 5
  }

  private var quickActions: Binding<[Action]> {
    Binding(
      get: { Array(model.visibleActions.prefix(numberOfQuickActions)) },
      set: {
        model.visibleActions.replaceSubrange(
          0..<min(numberOfQuickActions, model.visibleActions.count),
          with: $0
        )
      }
    )
  }

  private var listedActions: Binding<[Action]> {
    Binding(
      get: {
        if model.visibleActions.count < numberOfQuickActions {
          return []
        }
        return Array(model.visibleActions[numberOfQuickActions...])
      },
      set: {
        if model.visibleActions.count > numberOfQuickActions {
          model.visibleActions.replaceSubrange(numberOfQuickActions..., with: $0)
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
                .padding(.horizontal, 12)
                .background(Color(braveSystemName: .iosBrowserContainerHighlightIos), in: .capsule)
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
              }
            } icon: {
              Image(braveSystemName: "leo.product.vpn")
            }
          }
          .modifier(MenuRowButtonStyleModifier())
          .background(
            Color(braveSystemName: .iosBrowserContainerHighlightIos),
            in: .rect(cornerRadius: 14, style: .continuous)
          )
          .transition(.blurReplace())
        }
        Button {
          handlePresentation(.settings)
        } label: {
          Label(Strings.BrowserMenu.allSettingsButtonTitle, braveSystemImage: "leo.settings")
        }
        .modifier(MenuRowButtonStyleModifier())
        .background(
          Color(braveSystemName: .iosBrowserContainerHighlightIos),
          in: .rect(cornerRadius: 14, style: .continuous)
        )
      }
      .padding()
    }
    .background(Material.thick)
    .dynamicTypeSize(DynamicTypeSize.xSmall..<DynamicTypeSize.accessibility3)
    // Not sure if this is needed
    // .background(Color(braveSystemName: .materialThick))
    .sheet(isPresented: $isEditMenuPresented) {
      CustomizeMenuView(model: model)
    }
    .onGeometryChange(
      for: Bool.self,
      of: { $0.size.width <= 320 },
      action: { newValue in
        isHorizontalSpaceRestricted = newValue
      }
    )
  }
}

private struct QuickActionsView: View {
  @Binding var actions: [Action]
  var handler: (Binding<Action>) -> Void

  var body: some View {
    HStack(alignment: .top, spacing: 12) {
      ForEach($actions) { $action in
        let imageOverride: Image? = action.state.map {
          Image(braveSystemName: $0 ? "leo.toggle.on" : "leo.toggle.off")
        }
        Button {
          handler($action)
        } label: {
          Label {
            Text(action.title)
          } icon: {
            imageOverride ?? Image(braveSystemName: action.image)
          }
        }
        .modifier(ButtonStyleViewModifier(traits: action.traits))
        .disabled(action.attributes.contains(.disabled))
        .transition(.blurReplace())
        .id(action)
      }
    }
  }

  private struct ButtonStyleViewModifier: ViewModifier {
    var traits: Action.Traits

    func body(content: Content) -> some View {
      if #available(iOS 18, *) {
        content.buttonStyle(_OS18ButtonStyle(traits: traits))
      } else {
        content.buttonStyle(_StandardButtonStyle(traits: traits))
      }
    }

    @available(
      iOS,
      introduced: 18.0,
      message: """
        A PrimitiveButtonStyle to that handles highlights & action execution for quick action buttons.

        On iOS 18 there is a bug where a Button inside of a ScrollView which is being presented
        in a sheet will not cancel their tap gesture when a drag occurs which would move the sheet
        instead of the ScrollView, and as a result will execute the action you started your drag on
        even if you dismiss the sheet.

        This is tested up to iOS 18.2 and still broken.
        """
    )
    private struct _OS18ButtonStyle: PrimitiveButtonStyle {
      var traits: Action.Traits

      @GestureState private var isPressed: Bool = false

      func makeBody(configuration: Configuration) -> some View {
        configuration.label
          .simultaneousGesture(
            DragGesture(minimumDistance: 0).updating(
              $isPressed,
              body: { _, state, _ in state = true }
            )
          )
          .onTapGesture {
            configuration.trigger()
          }
          .labelStyle(_LabelStyle(isPressed: isPressed, traits: traits))
          .animation(isPressed ? nil : .default, value: isPressed)
      }
    }

    private struct _StandardButtonStyle: ButtonStyle {
      var traits: Action.Traits

      func makeBody(configuration: Configuration) -> some View {
        configuration.label
          .labelStyle(_LabelStyle(isPressed: configuration.isPressed, traits: traits))
      }
    }

  }

  private struct _LabelStyle: LabelStyle {
    @Environment(\.isEnabled) private var isEnabled
    @ScaledMetric private var iconFrameSize = 22
    @ScaledMetric private var iconFontSize = 18
    @ScaledMetric private var badgeRadius = 8
    var isPressed: Bool
    var traits: Action.Traits

    func makeBody(configuration: Configuration) -> some View {
      VStack(spacing: 8) {
        configuration.icon
          .frame(width: iconFrameSize, height: iconFrameSize)
          .padding(.vertical, 12)
          .font(.system(size: iconFontSize))
          .foregroundStyle(isEnabled ? Color(braveSystemName: .iconDefault) : .secondary)
          .frame(maxWidth: .infinity)
          .background {
            Color(braveSystemName: .iosBrowserContainerHighlightIos)
              .overlay(
                Color(braveSystemName: .iosBrowserContainerHighlightIos).opacity(isPressed ? 1 : 0)
              )
              .clipShape(.rect(cornerRadius: 14, style: .continuous))
              .hoverEffect()
          }
          .overlay(alignment: .topTrailing) {
            if let badgeColor = traits.badgeColor {
              Circle()
                .fill(Color(uiColor: badgeColor))
                .frame(width: badgeRadius, height: badgeRadius)
                .allowsHitTesting(false)
            }
          }
        configuration.title
          .font(.caption2)
          .lineLimit(2)
          .foregroundStyle(isEnabled ? Color(braveSystemName: .textPrimary) : .secondary)
          .multilineTextAlignment(.center)
      }
      .opacity(isEnabled ? 1 : 0.7)
      .contentShape(.rect)
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
    .modifier(MenuRowButtonStyleModifier())
    .disabled(action.attributes.contains(.disabled))
    .transition(.blurReplace())
    .id(action)
  }
}

private struct ActionsList: View {
  @Binding var actions: [Action]
  @Binding var additionalActions: [Action]
  var handler: (Binding<Action>) -> Void

  init(
    actions: Binding<[Action]>,
    additionalActions: Binding<[Action]>,
    handler: @escaping (Binding<Action>) -> Void
  ) {
    self._actions = actions
    self._additionalActions = additionalActions
    self.handler = handler
  }

  init(
    action: Action,
    handler: @escaping (Action) -> Void
  ) {
    self._actions = .constant([action])
    self._additionalActions = .constant([])
    self.handler = { action in
      handler(action.wrappedValue)
    }
  }

  @State private var isAdditionalActionsVisible: Bool = false
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
          .modifier(MenuRowButtonStyleModifier())
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
    .background(Color(braveSystemName: .iosBrowserContainerHighlightIos))
    .clipShape(.rect(cornerRadius: 14, style: .continuous))
  }
}

private struct MenuRowButtonStyleModifier: ViewModifier {
  func body(content: Content) -> some View {
    if #available(iOS 18, *) {
      content.buttonStyle(_OS18ButtonStyle())
    } else {
      content.buttonStyle(_StandardButtonStyle())
    }
  }

  @available(
    iOS,
    introduced: 18.0,
    message: """
      A PrimitiveButtonStyle to that handles highlights & action execution for menu row buttons.

      On iOS 18 there is a bug where a Button inside of a ScrollView which is being presented
      in a sheet will not cancel their tap gesture when a drag occurs which would move the sheet
      instead of the ScrollView, and as a result will execute the action you started your drag on
      even if you dismiss the sheet.

      This is tested up to iOS 18.2 and still broken.
      """
  )
  private struct _OS18ButtonStyle: PrimitiveButtonStyle {
    @GestureState private var isPressed: Bool = false

    func makeBody(configuration: Configuration) -> some View {
      configuration.label
        .frame(minHeight: 46)
        .padding(.horizontal, 16)
        .frame(maxWidth: .infinity, minHeight: 44, alignment: .leading)
        .contentShape(.rect)
        .simultaneousGesture(
          DragGesture(minimumDistance: 0).updating(
            $isPressed,
            body: { _, state, _ in state = true }
          )
        )
        .onTapGesture {
          configuration.trigger()
        }
        .hoverEffect()
        .background(
          Color(braveSystemName: .iosBrowserContainerHighlightIos).opacity(isPressed ? 1 : 0)
            .animation(isPressed ? nil : .default, value: isPressed)
        )
        .labelStyle(_LabelStyle())
    }
  }

  private struct _StandardButtonStyle: ButtonStyle {
    func makeBody(configuration: Configuration) -> some View {
      configuration.label
        .frame(minHeight: 46)
        .padding(.horizontal, 16)
        .frame(maxWidth: .infinity, minHeight: 44, alignment: .leading)
        .contentShape(.rect)
        .hoverEffect()
        .background(
          Color(braveSystemName: .iosBrowserContainerHighlightIos).opacity(
            configuration.isPressed ? 1 : 0
          )
        )
        .labelStyle(_LabelStyle())
    }
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
          .foregroundStyle(isEnabled ? Color(braveSystemName: .iconDefault) : .secondary)
        configuration.title
          .font(.body)
          .foregroundStyle(isEnabled ? Color(braveSystemName: .textPrimary) : .secondary)
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
