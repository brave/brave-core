// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import DesignSystem
import SwiftUI

extension AiChat.ActionGroup: Identifiable {
  public var id: String {
    self.category
  }
}

private struct AIChatSlashToolsHeaderView: View {
  @Binding
  var isExpanded: Bool

  var title: String

  var body: some View {
    HStack {
      Button(
        action: {
          isExpanded.toggle()
        },
        label: {
          HStack {
            Text(title.uppercased())
              .font(.caption2.weight(.semibold))
              .foregroundStyle(Color(braveSystemName: .textTertiary))
              .fixedSize(horizontal: false, vertical: true)
              .frame(maxWidth: .infinity, alignment: .leading)

            Image(braveSystemName: isExpanded ? "leo.carat.up" : "leo.carat.down")
              .tint(Color(braveSystemName: .iconDefault))
          }
          .padding()
        }
      )
    }
    .background(Color(braveSystemName: .pageBackground))
  }
}

private struct AIChatSlashToolsGroupView: View {
  var title: String
  var options: [AiChat.ActionEntry]

  @Binding
  var selectedOption: AiChat.ActionEntry?

  @State
  private var isExpanded = true

  var body: some View {
    AIChatSlashToolsHeaderView(isExpanded: $isExpanded, title: title)

    if isExpanded {
      LazyVStack {
        ForEach(Array(options.enumerated()), id: \.element) { index, option in
          Button(
            action: {
              selectedOption = option
            },
            label: {
              switch option.tag {
              case .subheading:
                VStack {
                  if index > 0 {
                    Color(braveSystemName: .dividerSubtle)
                      .frame(height: 1.0)
                      .padding(.horizontal)
                  }

                  Text(option.subheading?.uppercased() ?? "")
                    .font(.caption2.weight(.semibold))
                    .foregroundStyle(Color(braveSystemName: .textTertiary))
                    .fixedSize(horizontal: false, vertical: true)
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding()
                }
              case .details:
                Text(option.details?.label ?? "")
                  .font(.subheadline)
                  .foregroundStyle(Color(braveSystemName: .textPrimary))
                  .fixedSize(horizontal: false, vertical: true)
                  .frame(maxWidth: .infinity, alignment: .leading)
                  .padding()
              case .null:
                Text(Strings.AIChat.leoSlashToolsOptionNull)
                  .font(.subheadline)
                  .foregroundStyle(Color(braveSystemName: .textPrimary))
                  .fixedSize(horizontal: false, vertical: true)
                  .frame(maxWidth: .infinity, alignment: .leading)
                  .padding()
              @unknown default:
                Text(Strings.AIChat.leoSlashToolsOptionUnknown)
                  .font(.subheadline)
                  .foregroundStyle(Color(braveSystemName: .textPrimary))
                  .fixedSize(horizontal: false, vertical: true)
                  .frame(maxWidth: .infinity, alignment: .leading)
                  .padding()
              }
            }
          )
          .disabled(option.tag == .subheading)
        }
      }
    }
  }
}

struct AIChatSlashToolsView: View {
  @State
  private var contentSize: CGSize = .zero

  private var filteredSlashActions: [AiChat.ActionGroup] {
    let normalizeText = { (text: String) in
      return text.replacing(#/\s/#, with: "").uppercased()
    }

    if !prompt.hasPrefix("/") {
      return slashActions
    }

    let prompt = normalizeText(String(prompt.dropFirst()))
    if prompt.isEmpty {
      return slashActions
    }

    return slashActions.map({ group in
      let entries = group.entries.filter({
        if $0.tag == .subheading {
          return false
        }

        if $0.tag == .details {
          return normalizeText($0.details?.label ?? "").contains(
            prompt.trimmingCharacters(in: .whitespacesAndNewlines)
          )
        }

        return false
      })

      return AiChat.ActionGroup(category: group.category, entries: entries)
    })
    .filter({ !$0.entries.isEmpty })
  }

  @Binding
  var isShowing: Bool

  @Binding
  var prompt: String

  var slashActions: [AiChat.ActionGroup]

  @Binding
  var selectedOption:
    (
      group: AiChat.ActionGroup,
      entry: AiChat.ActionEntry
    )?

  var body: some View {
    ScrollView {
      VStack(spacing: 0.0) {
        ForEach(filteredSlashActions, id: \.category) { group in
          AIChatSlashToolsGroupView(
            title: group.category,
            options: group.entries,
            selectedOption:
              .init(
                get: { selectedOption?.entry },
                set: { entry in
                  if let entry = entry {
                    selectedOption = (group, entry)
                    prompt = ""
                    isShowing = false
                  }
                }
              )
          )

          Color(braveSystemName: .dividerSubtle)
            .frame(height: 1.0)
        }
      }
      .background(Color(braveSystemName: .containerBackground))
      .overlay {
        GeometryReader { proxy in
          Color.clear
            .onAppear {
              contentSize = proxy.size
            }
            .onChange(of: proxy.size) { size in
              contentSize = size
            }
        }
      }
    }
    .background(Color(.braveBackground))
    .clipShape(RoundedRectangle(cornerRadius: 8, style: .continuous))
    .shadow(color: .black.opacity(0.2), radius: 2, x: 0, y: 1)
    .frame(maxHeight: min(contentSize.height, 300.0))
    .padding()
    .scaleEffect(CGSize(width: 1.0, height: 1.0), anchor: .bottom)
  }
}

#if DEBUG
struct AIChatSlashToolsView_Preview: PreviewProvider {
  static var previews: some View {
    let slashActions = [
      AiChat.ActionGroup(
        category: "Quick actions",
        entries: [.init(details: .init(label: "Explain", type: .explain))]
      ),

      AiChat.ActionGroup(
        category: "Rewrite",
        entries: [
          .init(details: .init(label: "Paraphrase", type: .paraphrase)),
          .init(details: .init(label: "Improve", type: .improve)),
          .init(subheading: "Change tone"),
          .init(details: .init(label: "Change tone / Academic", type: .academicize)),
          .init(details: .init(label: "Change tone / Professional", type: .professionalize)),
          .init(details: .init(label: "Change tone / Persuasive", type: .persuasiveTone)),
          .init(details: .init(label: "Change tone / Casual", type: .casualize)),
          .init(details: .init(label: "Change tone / Funny", type: .funnyTone)),
          .init(details: .init(label: "Change tone / Shorten", type: .shorten)),
          .init(details: .init(label: "Change tone / Expand", type: .expand)),
        ]
      ),

      AiChat.ActionGroup(
        category: "Create",
        entries: [
          .init(details: .init(label: "Tagline", type: .createTagline)),
          .init(subheading: "Social media"),
          .init(
            details: .init(label: "Social media / Short post", type: .createSocialMediaCommentShort)
          ),
          .init(
            details: .init(label: "Social media / Long post", type: .createSocialMediaCommentLong)
          ),
        ]
      ),
    ]
    return AIChatSlashToolsView(
      isShowing: .constant(false),
      prompt: .constant(""),
      slashActions: slashActions,
      selectedOption: .constant(nil)
    )
    .previewLayout(.sizeThatFits)
  }
}
#endif
