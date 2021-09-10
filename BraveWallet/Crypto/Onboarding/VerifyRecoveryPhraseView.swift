/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftUI
import BraveUI

@available(iOS 14.0, *)
struct VerifyRecoveryPhraseView: View {
  @ObservedObject var keyringStore: KeyringStore
  
  @State private var recoveryWords: [RecoveryWord] = []
  @State private var randomizedWords: [RecoveryWord] = []
  @State private var selectedWords: [RecoveryWord] = []
  
  @Environment(\.modalPresentationMode) @Binding private var modalPresentationMode
  
  private var wordsSelectedInCorrectOrder: Bool {
    recoveryWords == selectedWords
  }
  
  private func tappedWord(_ word: RecoveryWord) {
    withAnimation(.default) {
      selectedWords.append(word)
    }
  }
  
  private func tappedVerify() {
    guard wordsSelectedInCorrectOrder else { return }
    keyringStore.notifyWalletBackupComplete()
    if keyringStore.isOnboardingVisible {
      keyringStore.markOnboardingCompleted()
    } else {
      modalPresentationMode = false
    }
  }
  
  var body: some View {
    ScrollView(.vertical) {
      VStack(spacing: 16) {
        Group {
          Text("Verify recovery phrase") // NSLocalizedString
            .font(.headline)
          Text("Tap the words to put them next to each other in the correct order.") // NSLocalizedString
            .font(.subheadline)
            .foregroundColor(.secondary)
        }
        .fixedSize(horizontal: false, vertical: true)
        .multilineTextAlignment(.center)
        SelectedWordsBox(recoveryWords: recoveryWords, selectedWords: $selectedWords)
        RecoveryPhraseGrid(data: randomizedWords, id: \.id) { word in
          let selected = selectedWords.contains(word)
          Button(action: {
            tappedWord(word)
          }) {
            Text(verbatim: word.value)
              .font(.footnote.bold())
              .foregroundColor(.primary)
              .fixedSize(horizontal: false, vertical: true)
              .padding(8)
              .frame(maxWidth: .infinity)
          }
          .background(
            Color(.braveDisabled)
              .clipShape(RoundedRectangle(cornerRadius: 4, style: .continuous))
          )
          .opacity(selected ? 0.0 : 1.0)
          .background(
            Group {
              if selected {
                RoundedRectangle(cornerRadius: 4, style: .continuous)
                  .inset(by: 1)
                  .stroke(Color.black.opacity(0.1))
              } else {
                Color.clear
              }
            }
          )
        }
        .padding()
        Button(action: tappedVerify) {
          Text("Verify") // NSLocalizedString
        }
        .buttonStyle(BraveFilledButtonStyle(size: .normal))
        .disabled(!wordsSelectedInCorrectOrder)
        .animation(.linear(duration: 0.15), value: wordsSelectedInCorrectOrder)
        if keyringStore.isOnboardingVisible {
          Button(action: {
            keyringStore.markOnboardingCompleted()
          }) {
            Text("Skip") // NSLocalizedString
              .font(Font.subheadline.weight(.medium))
              .foregroundColor(Color(.braveLabel))
          }
        }
      }
      .padding()
    }
    .onAppear {
      keyringStore.recoveryPhrase { words in
        recoveryWords = words
        randomizedWords = words.shuffled()
      }
    }
    .background(Color(.braveBackground).edgesIgnoringSafeArea(.all))
    .navigationTitle("Crypto") // NSLocalizedString
    .navigationBarTitleDisplayMode(.inline)
  }
}

@available(iOS 14.0, *)
private struct SelectedWordsBox: View {
  var recoveryWords: [RecoveryWord]
  @Binding var selectedWords: [RecoveryWord]
  @Environment(\.pixelLength) private var pixelLength
  
  enum WordEntry: Hashable, Identifiable {
    case word(String, index: Int, isCorrect: Bool)
    case placeholder(atIndex: Int)
    
    func hash(into hasher: inout Hasher) {
      switch self {
      case .word(let word, let index, _):
        hasher.combine(word)
        hasher.combine(index)
      case .placeholder(let index): hasher.combine(index)
      }
    }
    
    var id: String {
      switch self {
      case .word(let word, let index, _):
        return "\(word)-\(index)"
      case .placeholder(let index):
        return "placeholder-\(index)"
      }
    }
  }
  
  private var entries: [WordEntry] {
    var words: [WordEntry] =
      selectedWords
        .enumerated()
        .map({ .word($0.element.value, index: $0.offset, isCorrect: recoveryWords[$0.offset] == $0.element) })
    if words.count < 12 {
      words.append(contentsOf: (words.count..<12).map { .placeholder(atIndex: $0) })
    }
    return words
  }
  
  private func tappedWord(atIndex index: Int) {
    withAnimation(.default) {
      _ = selectedWords.remove(at: index)
    }
  }
  
  private func view(for entry: WordEntry) -> some View {
    let clipShape = RoundedRectangle(cornerRadius: 4, style: .continuous)
    return Group {
      switch entry {
      case .placeholder:
        Text("Word")
          .padding(8)
          .frame(maxWidth: .infinity)
          .hidden()
      case .word(let word, let index, let isCorrect):
        Button(action: { tappedWord(atIndex: index) }) {
          Text(verbatim: word)
            .padding(8)
            .frame(maxWidth: .infinity)
            .fixedSize(horizontal: false, vertical: true)
            .foregroundColor(isCorrect ? .primary : .red)
            .overlay(
              clipShape
                .stroke(Color.black.opacity(0.1), lineWidth: pixelLength * 2)
            )
            .clipShape(clipShape)
        }
        .background(Color(.braveDisabled).clipShape(RoundedRectangle(cornerRadius: 4, style: .continuous)))
      }
    }
    .font(.footnote.bold())
  }
  
  var body: some View {
    RecoveryPhraseGrid(data: entries, id: \.id) { word in
      view(for: word)
    }
    .padding(8)
    .background(
      Color.gray
        .clipShape(
          RoundedRectangle(cornerRadius: 8, style: .continuous)
            .inset(by: pixelLength / 2)
            .stroke(lineWidth: pixelLength / 2)
        )
    )
    .navigationTitle("Crypto") // NSLocalizedString
    .navigationBarTitleDisplayMode(.inline)
    .introspectViewController { vc in
      vc.navigationItem.backButtonTitle = "Verify Phrase" // NSLocalizedString
      vc.navigationItem.backButtonDisplayMode = .minimal
    }
  }
}

#if DEBUG
@available(iOS 14.0, *)
struct VerifyRecoveryPhraseView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      VerifyRecoveryPhraseView(keyringStore: .previewStore)
    }
    .previewLayout(.sizeThatFits)
    .previewColorSchemes()
  }
}
#endif
