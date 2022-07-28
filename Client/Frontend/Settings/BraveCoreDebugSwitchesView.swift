// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveShared
import BraveCore

extension BraveCoreSwitch {
  fileprivate var displayString: String {
    switch self {
    case .vModule:
      return "Log Verbosity"
    case .componentUpdater:
      return "Component Updater"
    case .syncURL:
      return "Sync URL"
    default:
      return ""
    }
  }
}

private enum SkusEnvironment: String, CaseIterable {
  case development
  case staging
  case production
}

private struct BasicStringInputView: View {
  @ObservedObject private var activeSwitches = Preferences.BraveCore.activeSwitches
  @ObservedObject private var switchValues = Preferences.BraveCore.switchValues
  @Environment(\.presentationMode) @Binding private var presentationMode

  var coreSwitch: BraveCoreSwitch
  var hint: String?

  @State private var text: String = ""

  var body: some View {
    List {
      Section {
        TextField(coreSwitch.displayString, text: $text)
          .disableAutocorrection(true)
          .autocapitalization(.none)
      } footer: {
        if let hint = hint {
          Text(hint)
        }
      }
    }
    .listStyle(.insetGrouped)
    .navigationTitle(coreSwitch.displayString)
    .onAppear {
      // SwiftUI bug, has to wait a bit
      DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
        text = switchValues.value[coreSwitch.rawValue, default: ""]
      }
    }
    .toolbar {
      ToolbarItemGroup(placement: .confirmationAction) {
        Button {
          if text.isEmpty {
            switchValues.value[coreSwitch.rawValue] = nil
            activeSwitches.value.removeAll(where: { $0 == coreSwitch.rawValue })
          } else {
            switchValues.value[coreSwitch.rawValue] = text
            if !activeSwitches.value.contains(coreSwitch.rawValue) {
              activeSwitches.value.append(coreSwitch.rawValue)
            }
          }
          presentationMode.dismiss()
        } label: {
          Text("Save")
            .foregroundColor(Color(.braveOrange))
        }
      }
    }
  }
}

private struct BasicPickerInputView: View {
  @ObservedObject private var activeSwitches = Preferences.BraveCore.activeSwitches
  @ObservedObject private var switchValues = Preferences.BraveCore.switchValues
  @Environment(\.presentationMode) @Binding private var presentationMode

  var coreSwitch: BraveCoreSwitch
  var options: [String]

  @State private var selectedItem: String = ""

  var body: some View {
    List {
      Picker("", selection: $selectedItem) {
        Text("Default")
          .foregroundColor(Color(.secondaryBraveLabel))
          .tag("")
        ForEach(options, id: \.self) { option in
          Text(option.capitalized).tag(option)
        }
      }
      .pickerStyle(.inline)
    }
    .listStyle(.insetGrouped)
    .navigationTitle(coreSwitch.displayString)
    .onAppear {
      // SwiftUI bug, has to wait a bit
      DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
        selectedItem = switchValues.value[coreSwitch.rawValue, default: ""]
      }
    }
    .toolbar {
      ToolbarItemGroup(placement: .confirmationAction) {
        Button {
          if selectedItem.isEmpty {
            switchValues.value[coreSwitch.rawValue] = nil
            activeSwitches.value.removeAll(where: { $0 == coreSwitch.rawValue })
          } else {
            switchValues.value[coreSwitch.rawValue] = selectedItem
            if !activeSwitches.value.contains(coreSwitch.rawValue) {
              activeSwitches.value.append(coreSwitch.rawValue)
            }
          }
          presentationMode.dismiss()
        } label: {
          Text("Save")
            .foregroundColor(Color(.braveOrange))
        }
      }
    }
  }
}

struct BraveCoreDebugSwitchesView: View {
  @ObservedObject private var activeSwitches = Preferences.BraveCore.activeSwitches
  @ObservedObject private var switchValues = Preferences.BraveCore.switchValues

  private struct SwitchContainer: View {
    @ObservedObject private var activeSwitches = Preferences.BraveCore.activeSwitches
    @ObservedObject private var switchValues = Preferences.BraveCore.switchValues

    var coreSwitch: BraveCoreSwitch

    init(_ coreSwitch: BraveCoreSwitch) {
      self.coreSwitch = coreSwitch
    }

    private var binding: Binding<Bool> {
      .init(
        get: {
          activeSwitches.value.contains(coreSwitch.rawValue) && !switchValues.value[coreSwitch.rawValue, default: ""].isEmpty
        },
        set: { isOn in
          if isOn {
            activeSwitches.value.append(coreSwitch.rawValue)
          } else {
            activeSwitches.value.removeAll(where: { $0 == coreSwitch.rawValue })
          }
        }
      )
    }

    var body: some View {
      HStack(spacing: 16) {
        Toggle(coreSwitch.displayString, isOn: binding)
          .labelsHidden()
        VStack(alignment: .leading) {
          HStack {
            Text(coreSwitch.displayString)
              .font(.headline)
          }
          VStack(alignment: .leading, spacing: 4) {
            Text(coreSwitch.rawValue)
              .font(.footnote)
              .foregroundColor(Color(.secondaryBraveLabel))
            if let value = switchValues.value[coreSwitch.rawValue], !value.isEmpty {
              Text("\(Image(systemName: "equal.square.fill")) \(value)")
                .font(.caption)
                .foregroundColor(binding.wrappedValue ? Color(.braveBlurpleTint) : Color(.secondaryBraveLabel))
                .lineLimit(1)
            }
          }
        }
      }
      .padding(.vertical, 6)
    }
  }

  var body: some View {
    List {
      Section {
        Text("Switches only affect fresh launches")
          .frame(maxWidth: .infinity)
          .font(.footnote)
          .foregroundColor(Color(.braveLabel))
          .listRowBackground(Color(.braveGroupedBackground))
          .listRowInsets(.zero)
      }
      Section {
        // Sync URL
        NavigationLink {
          BasicStringInputView(coreSwitch: .syncURL)
            .keyboardType(.URL)
        } label: {
          SwitchContainer(.syncURL)
        }
        NavigationLink {
          BasicStringInputView(coreSwitch: .componentUpdater, hint: "Should match the format: url-source={url}")
        } label: {
          SwitchContainer(.componentUpdater)
        }
        NavigationLink {
          BasicStringInputView(coreSwitch: .vModule, hint: "Should match the format:\n\n{folder-expression}={level}\n\nDefaults to */brave/*=5")
        } label: {
          SwitchContainer(.vModule)
        }
      }
      Section {
        Button("Disable All") {
          withAnimation {
            activeSwitches.value = []
          }
        }
      }
    }
    .listStyle(.insetGrouped)
    .navigationBarTitle("BraveCore Switches")
  }
}

#if DEBUG
struct BraveCoreDebugSwitchesView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      BraveCoreDebugSwitchesView()
        .navigationBarTitleDisplayMode(.inline)
    }
  }
}
#endif
