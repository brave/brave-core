// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import Preferences
import BraveCore
import BraveUI

extension BraveCoreSwitchKey {
  fileprivate var displayString: String {
    switch self {
    case .vModule:
      return "Log Verbosity"
    case .componentUpdater:
      return "Component Updater"
    case .syncURL:
      return "Sync URL"
    case .p3aDoNotRandomizeUploadInterval:
      return "Don't Randomize Upload Interval"
    case .p3aIgnoreServerErrors:
      return "Ignore Server Errors"
    case .p3aUploadIntervalSeconds:
      return "Upload Interval"
    case .p3aTypicalRotationIntervalSeconds:
      return "Typical Rotation Interval"
    case .p3aExpressRotationIntervalSeconds:
      return "Express Rotation Interval"
    case .p3aJsonUploadServerURL:
      return "Json Upload Server URL"
    case .enableFeatures:
      return "Enable Features"
    default:
      return ""
    }
  }
  /// Whether or not the key is passed in without a value
  public var isValueless: Bool {
    switch self {
    case .p3aDoNotRandomizeUploadInterval, .p3aIgnoreServerErrors:
      return true
    default:
      return false
    }
  }
  
  static let enableFeatures: Self = .init(rawValue: "enable-features")
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

  var coreSwitch: BraveCoreSwitchKey
  var hint: String?

  @State private var text: String = ""

  var body: some View {
    List {
      Section {
        TextField(coreSwitch.displayString, text: $text)
          .disableAutocorrection(true)
          .autocapitalization(.none)
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } footer: {
        if let hint = hint {
          Text(hint)
        }
      }
    }
    .listStyle(.insetGrouped)
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
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
            .foregroundColor(Color(.braveBlurpleTint))
        }
      }
    }
  }
}

private struct BasicPickerInputView: View {
  @ObservedObject private var activeSwitches = Preferences.BraveCore.activeSwitches
  @ObservedObject private var switchValues = Preferences.BraveCore.switchValues
  @Environment(\.presentationMode) @Binding private var presentationMode

  var coreSwitch: BraveCoreSwitchKey
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
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
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
            .foregroundColor(Color(.braveBlurpleTint))
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

    var coreSwitch: BraveCoreSwitchKey

    init(_ coreSwitch: BraveCoreSwitchKey) {
      self.coreSwitch = coreSwitch
    }

    private var binding: Binding<Bool> {
      .init(
        get: {
          activeSwitches.value.contains(coreSwitch.rawValue) && (coreSwitch.isValueless || !switchValues.value[coreSwitch.rawValue, default: ""].isEmpty)
        },
        set: { isOn in
          if !coreSwitch.isValueless && switchValues.value[coreSwitch.rawValue, default: ""].isEmpty {
            return
          }
          var switches = Set(activeSwitches.value)
          if isOn {
            switches.insert(coreSwitch.rawValue)
          } else {
            switches.remove(coreSwitch.rawValue)
          }
          activeSwitches.value = Array(switches)
        }
      )
    }

    var body: some View {
      HStack(spacing: 16) {
        Toggle(coreSwitch.displayString, isOn: binding)
          .toggleStyle(SwitchToggleStyle(tint: Color(.braveBlurpleTint)))
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
      .padding(.vertical, 2)
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
        Group {
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
          NavigationLink {
            BasicStringInputView(coreSwitch: .enableFeatures, hint: "Should match the format:\n\n{feature_name}\n\nMultiple features can be enabled via comma separation")
          } label: {
            SwitchContainer(.enableFeatures)
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      Section {
        Group {
          SwitchContainer(.p3aDoNotRandomizeUploadInterval)
          SwitchContainer(.p3aIgnoreServerErrors)
          NavigationLink {
            BasicStringInputView(coreSwitch: .p3aUploadIntervalSeconds, hint: "Overrides the number of seconds to upload P3A metrics")
          } label: {
            SwitchContainer(.p3aUploadIntervalSeconds)
          }
          NavigationLink {
            BasicStringInputView(coreSwitch: .p3aJsonUploadServerURL, hint: "Overrides the P3A cloud backend URL.")
          } label: {
            SwitchContainer(.p3aJsonUploadServerURL)
          }
          NavigationLink {
            BasicStringInputView(coreSwitch: .p3aTypicalRotationIntervalSeconds, hint: "Interval in seconds between restarting the uploading process for all gathered values")
          } label: {
            SwitchContainer(.p3aTypicalRotationIntervalSeconds)
          }
          NavigationLink {
            BasicStringInputView(coreSwitch: .p3aExpressRotationIntervalSeconds, hint: "Interval in seconds between restarting the uploading process for all gathered values")
          } label: {
            SwitchContainer(.p3aExpressRotationIntervalSeconds)
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } header: {
        Text("P3A")
      }
      Section {
        Button("Disable All") {
          withAnimation {
            activeSwitches.value = []
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
    }
    .listStyle(.insetGrouped)
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
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
