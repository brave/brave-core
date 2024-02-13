// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveUI

/// A small file browser that lets you inspect the contents of the apps sandbox.
///
/// Currently it only supports viewing the sizes and file names
struct SandboxInspectorView: View {
  private struct Node: Identifiable {
    var url: URL
    var size: Int
    var isDirectory: Bool
    var children: [Node]?

    var id: String {
      url.path
    }
  }

  @State private var nodes: [Node] = []
  @State private var total: String = ""
  @State private var isLoading: Bool = false

  private let formatter = ByteCountFormatter().then {
    $0.countStyle = .file
    $0.allowsNonnumericFormatting = false
    $0.allowedUnits = [.useKB, .useMB, .useGB, .useTB]
  }

  private func getNodes() {
    let fm = FileManager.default
    let base = URL(fileURLWithPath: NSHomeDirectory())
    func nodes(from base: URL) -> [Node]? {
      do {
        return try fm.contentsOfDirectory(
          at: base,
          includingPropertiesForKeys: nil
        ).compactMap { url -> Node? in
          var isDirectory: ObjCBool = false
          guard fm.fileExists(atPath: url.path, isDirectory: &isDirectory) else {
            return nil
          }
          let size: Int
          var children: [Node]?
          if isDirectory.boolValue {
            // TODO: Optimize this better, enumerator(at:includingPropertiesForKeys:) already includes every subitem URL, we should construct the entire tree using it
            if let subitems =
              fm.enumerator(
                at: url,
                includingPropertiesForKeys: nil
              )?.allObjects as? [URL], !subitems.isEmpty {
              size = try subitems.reduce(0, {
                $0
                + (try
                   $1.resourceValues(forKeys: [.totalFileAllocatedSizeKey])
                  .totalFileAllocatedSize ?? 0)
              })
              children = nodes(from: url)
            } else {
              size = 0
            }
          } else {
            size =
              try
              url.resourceValues(forKeys: [.totalFileAllocatedSizeKey])
              .totalFileAllocatedSize ?? 0
          }
          return Node(
            url: url,
            size: size,
            isDirectory: isDirectory.boolValue,
            children: children?.sorted(by: { $0.size > $1.size })
          )
        }
      } catch {
        print("Error: \(error.localizedDescription)")
        return nil
      }
    }

    self.isLoading = true
    DispatchQueue.global(qos: .userInitiated).async {
      let nodes = nodes(from: base) ?? []
      DispatchQueue.main.async {
        self.nodes = nodes.sorted(by: { $0.size > $1.size })
        self.total = formatter.string(fromByteCount: Int64(self.nodes.reduce(0, { $0 + $1.size })))
        self.isLoading = false
      }
    }
  }

  var body: some View {
    List {
      Section {
        HStack {
          Text("Total Size")
          Spacer()
          Text(total)
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      Section {
        if isLoading {
          ProgressView()
            .progressViewStyle(CircularProgressViewStyle())
            .frame(maxWidth: .infinity)
        } else {
          OutlineGroup(nodes, children: \.children) { row in
            HStack {
              Label {
                Text(row.url.lastPathComponent)
              } icon: {
                Image(braveSystemName: "leo.folder")
                  .opacity(row.isDirectory ? 1 : 0)
              }
              Spacer()
              Text(formatter.string(fromByteCount: Int64(row.size)))
            }
            .font(.callout)
          }
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .navigationTitle("Sandbox Inspector")
    .navigationBarTitleDisplayMode(.inline)
    .onAppear(perform: getNodes)
  }
}
