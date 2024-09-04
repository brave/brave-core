// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

struct TranslateSettingsView: View {
  
  var body: some View {
    NavigationStack {
      List {
        Section {
          NavigationLink {
            
          } label: {
            LabeledContent("From", value: "Selected Language")
          }
          
          NavigationLink {
            
          } label: {
            LabeledContent("To", value: "Selected Language")
          }
        } footer: {
          VStack {
            Button {
              
            } label: {
              Text("Translate")
                .font(.body.weight(.semibold))
                .padding()
                .frame(maxWidth: .infinity)
                .foregroundStyle(Color(braveSystemName: .schemesOnPrimary))
                .background(
                  ContainerRelativeShape()
                    .fill(Color(braveSystemName: .buttonBackground))
                )
                .containerShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
            }
            .buttonStyle(.plain)
            
            Button {
              
            } label: {
              Text("Show Original")
                .font(.body.weight(.semibold))
                .padding()
                .frame(maxWidth: .infinity)
                .foregroundStyle(Color(braveSystemName: .textSecondary))
                .background(
                  ContainerRelativeShape()
                    .fill(Color(.clear))
                )
                .containerShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
            }
            .buttonStyle(.plain)
          }
          .listRowInsets(.init())
          .padding(.top)
        }
        .navigationBarTitle("Translate")
        .navigationBarTitleDisplayMode(.inline)
      }
    }
  }
}

#Preview {
  TranslateSettingsView()
}
