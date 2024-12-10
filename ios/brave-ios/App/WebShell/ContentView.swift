// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI

private let braveCoreMain: BraveCoreMain = {
  let main = BraveCoreMain(userAgent: nil)
  main.scheduleLowPriorityStartupTasks()
  return main
}()

@dynamicMemberLookup
class WebViewModel: ObservableObject {
  @Published var webView: CWVWebView {
    didSet {
      setUpObservers()
    }
  }

  init(webView: CWVWebView) {
    self.webView = webView
    setUpObservers()
  }

  private func setUpObservers() {
    func subscriber<Value>(for keyPath: KeyPath<CWVWebView, Value>) -> NSKeyValueObservation {
      return webView.observe(keyPath, options: [.prior]) { _, change in
        if change.isPrior {
          self.objectWillChange.send()
        }
      }
    }
    observers = [
      subscriber(for: \.title),
      subscriber(for: \.visibleURL),
      subscriber(for: \.visibleLocationString),
      subscriber(for: \.lastCommittedURL),
      subscriber(for: \.isLoading),
      subscriber(for: \.estimatedProgress),
      subscriber(for: \.visibleSSLStatus),
      subscriber(for: \.canGoBack),
      subscriber(for: \.canGoForward),
    ]
  }

  private var observers: [NSKeyValueObservation] = []

  public subscript<T>(dynamicMember keyPath: KeyPath<CWVWebView, T>) -> T {
    webView[keyPath: keyPath]
  }
}

struct WebView: UIViewRepresentable {
  var webView: CWVWebView

  func makeUIView(context: Context) -> CWVWebView {
    webView
  }
  func updateUIView(_ uiView: CWVWebView, context: Context) {
  }
}

struct ContentView: View {
  @StateObject private var model: WebViewModel = .init(
    webView: .init(frame: .zero, configuration: braveCoreMain.defaultWebViewConfiguration)
  )
  @State private var isPresentingAlert: Bool = false
  @State private var urlSubmitText: String = ""

  var body: some View {
    NavigationStack {
      VStack(spacing: 0) {
        WebView(webView: model.webView)
        Divider()
        VStack {
          Button {
            isPresentingAlert = true
          } label: {
            HStack {
              Group {
                if let status = model.visibleSSLStatus {
                  switch status.securityStyle {
                  case .unknown:
                    Image(systemName: "exclamationmark.circle.fill")
                  case .authenticationBroken:
                    Image(systemName: "exclamationmark.octagon.fill")
                  case .unauthenticated:
                    Image(systemName: "exclamationmark.triangle.fill")
                  case .authenticated:
                    if !status.hasOnlySecureContent {
                      Image(systemName: "exclamationmark.triangle")
                    } else {
                      EmptyView()
                    }
                  @unknown default:
                    EmptyView()
                  }
                }
              }
              .font(.caption)
              VStack {
                Text(model.visibleLocationString)
                  .font(.callout)
                if let visibleURL = model.visibleURL,
                  visibleURL.absoluteString != model.visibleLocationString
                {
                  Text(visibleURL.absoluteString)
                    .font(.caption)
                    .lineLimit(1)
                }
              }
            }
            .foregroundStyle(.primary)
          }
          HStack(spacing: 16) {
            Button {
              model.webView.goBack()
            } label: {
              Image(systemName: "chevron.left")
            }
            .disabled(!model.canGoBack)
            Button {
              model.webView.goForward()
            } label: {
              Image(systemName: "chevron.right")
            }
            .disabled(!model.canGoForward)
            Spacer()
            Button {
              model.webView.reload()
            } label: {
              Image(systemName: "arrow.clockwise")
            }
          }
        }
        .padding(.horizontal, 16)
        .padding(.vertical, 8)
        .imageScale(.large)
        .background(Material.bar)
      }
    }
    .alert(
      "URL",
      isPresented: $isPresentingAlert,
      actions: {
        TextField("URL or search term", text: $urlSubmitText)
          .autocorrectionDisabled()
          .textInputAutocapitalization(.never)
          .keyboardType(.URL)
        Button("Cancel", role: .cancel) {
          urlSubmitText = ""
        }
        Button("Submit") {
          let classification = AutocompleteClassifier.classify(urlSubmitText)
          if let url = classification?.destinationURL {
            model.webView.load(URLRequest(url: url))
          }
          urlSubmitText = ""
        }
      },
      message: {

      }
    )
    .onAppear {
      DispatchQueue.main.async {
        model.webView.load(URLRequest(url: URL(string: "https://search.brave.com")!))
      }
    }
  }
}

#Preview {
  ContentView()
}
