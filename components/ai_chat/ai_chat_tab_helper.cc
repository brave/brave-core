/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/ai_chat_tab_helper.h"

#include <queue>
#include <memory>
#include <string>
#include <utility>

#include "base/containers/contains.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/ai_chat/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/constants.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "ui/accessibility/ax_node.h"
#include "ui/accessibility/ax_tree.h"
#include "ui/base/l10n/l10n_util.h"
#include "net/base/load_flags.h"
#include "net/http/http_request_headers.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

using ai_chat::mojom::CharacterType;
using ai_chat::mojom::ConversationTurn;
using ai_chat::mojom::ConversationTurnVisibility;

namespace {

const char16_t kYoutubeTranscriptExtractionScript[] =
    uR"JS(
        console.log('Starting youtube transcript extraction')
        const url = ytplayer.config?.args.raw_player_response.captions.playerCaptionsTracklistRenderer.captionTracks[0].baseUrl
        console.log('got url', url)
        url
    )JS";

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("ai_chat", R"(
      semantics {
        sender: "AI Chat"
        description:
          "This is used to fetch video transcript"
          "on behalf of the user interacting with the ChatUI."
        trigger:
          "Triggered by user asking for a summary."
        data:
          "Will generate a text that attempts to match the user gave it"
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        policy_exception_justification:
          "Not implemented."
      }
    )");
}


static const ax::mojom::Role kContentRoles[] = {
    ax::mojom::Role::kHeading,
    ax::mojom::Role::kParagraph,
};

static const ax::mojom::Role kRolesToSkip[] = {
    ax::mojom::Role::kAudio,
    ax::mojom::Role::kBanner,
    ax::mojom::Role::kButton,
    ax::mojom::Role::kComplementary,
    ax::mojom::Role::kContentInfo,
    ax::mojom::Role::kFooter,
    ax::mojom::Role::kFooterAsNonLandmark,
    ax::mojom::Role::kImage,
    ax::mojom::Role::kLabelText,
    ax::mojom::Role::kNavigation,
    /* input elements */
    ax::mojom::Role::kTextField,
    ax::mojom::Role::kTextFieldWithComboBox,
    ax::mojom::Role::kComboBoxSelect,
    ax::mojom::Role::kListBox,
    ax::mojom::Role::kListBoxOption,
    ax::mojom::Role::kCheckBox,
    ax::mojom::Role::kRadioButton,
    ax::mojom::Role::kSlider,
    ax::mojom::Role::kSpinButton,
    ax::mojom::Role::kSearchBox,
};

void GetContentRootNodes(const ui::AXNode* root,
                         std::vector<const ui::AXNode*>* content_root_nodes) {
  std::queue<const ui::AXNode*> queue;
  queue.push(root);
  while (!queue.empty()) {
    const ui::AXNode* node = queue.front();
    queue.pop();
    // If a main or article node is found, add it to the list of content root
    // nodes and continue. Do not explore children for nested article nodes.
    if (node->GetRole() == ax::mojom::Role::kMain ||
        node->GetRole() == ax::mojom::Role::kArticle) {
      content_root_nodes->push_back(node);
      continue;
    }
    for (auto iter = node->UnignoredChildrenBegin();
         iter != node->UnignoredChildrenEnd(); ++iter) {
      queue.push(iter.get());
    }
  }
}

void AddContentNodesToVector(const ui::AXNode* node,
                             std::vector<const ui::AXNode*>* content_nodes) {
  if (base::Contains(kContentRoles, node->GetRole())) {
    content_nodes->emplace_back(node);
    return;
  }
  if (base::Contains(kRolesToSkip, node->GetRole())) {
    return;
  }
  for (auto iter = node->UnignoredChildrenBegin();
       iter != node->UnignoredChildrenEnd(); ++iter) {
    AddContentNodesToVector(iter.get(), content_nodes);
  }
}

void AddTextNodesToVector(const ui::AXNode* node,
                          std::vector<std::u16string>* strings) {
  const ui::AXNodeData& node_data = node->data();

  if (base::Contains(kRolesToSkip, node_data.role)) {
    return;
  }

  if (node_data.role == ax::mojom::Role::kStaticText) {
    if (node_data.HasStringAttribute(ax::mojom::StringAttribute::kName)) {
      strings->push_back(
          node_data.GetString16Attribute(ax::mojom::StringAttribute::kName));
    }
    return;
  }

  for (const auto* child : node->children()) {
    AddTextNodesToVector(child, strings);
  }
}
}  // namespace

AIChatTabHelper::AIChatTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<AIChatTabHelper>(*web_contents) {
  ai_chat_api_ =
      std::make_unique<AIChatAPI>(web_contents->GetBrowserContext()
                                      ->GetDefaultStoragePartition()
                                      ->GetURLLoaderFactoryForBrowserProcess());
}

AIChatTabHelper::~AIChatTabHelper() = default;

const std::vector<ConversationTurn>& AIChatTabHelper::GetConversationHistory() {
  return chat_history_;
}

const std::string& AIChatTabHelper::GetConversationHistoryString() {
  std::vector<std::string> turn_strings;
  for (const ConversationTurn& turn : chat_history_) {
    turn_strings.push_back((turn.character_type == CharacterType::HUMAN
                                ? ai_chat::kHumanPromptPlaceholder
                                : ai_chat::kAIPromptPlaceholder) +
                           turn.text);
  }

  history_text_ = base::JoinString(turn_strings, "");

  return history_text_;
}

void AIChatTabHelper::AddToConversationHistory(const ConversationTurn& turn) {
  chat_history_.push_back(turn);

  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
  }
}

void AIChatTabHelper::UpdateOrCreateLastAssistantEntry(
    const std::string& updated_text) {
  if (chat_history_.empty() ||
      chat_history_.back().character_type != CharacterType::ASSISTANT) {
    AddToConversationHistory({CharacterType::ASSISTANT,
                              ConversationTurnVisibility::VISIBLE,
                              updated_text});
  } else {
    chat_history_.back().text = updated_text;
  }

  // Trigger an observer update to refresh the UI.
  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
  }
}

void AIChatTabHelper::SetArticleSummaryString(const std::string& text) {
  article_summary_ = text;
}

void AIChatTabHelper::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);

  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
    obs.OnAPIRequestInProgress(IsRequestInProgress());
  }
}

void AIChatTabHelper::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void AIChatTabHelper::RequestSummary() {
  if (!article_summary_.empty()) {
    VLOG(1) << __func__ << " Article summary is in cache\n";

    AddToConversationHistory({
        CharacterType::ASSISTANT,
        ConversationTurnVisibility::VISIBLE,
        article_summary_,
    });
    return;
  }

  auto* primary_rfh = web_contents()->GetPrimaryMainFrame();

  if (!primary_rfh) {
    // TODO(petemill): Don't allow the UI to submit requests at this state
    VLOG(1) << "Summary request submitted for a WebContents without a "
               "primary main frame";
    for (auto& obs : observers_) {
      obs.OnRequestSummaryFailed();
    }
    return;
  }

  ui::AXTreeID tree_id = web_contents()->GetPrimaryMainFrame()->GetAXTreeID();
  content::RenderFrameHost* rfh =
      content::RenderFrameHost::FromAXTreeID(tree_id);

  // Special case for YouTube videos transcripts
  // TODO(petemill): better hostname compare
  if (base::CompareCaseInsensitiveASCII(web_contents()->GetLastCommittedURL().host(), "www.youtube.com") == 0) {
    // TODO(petemill): Refactor to VideoTranscriptFetcher
    DVLOG(2) << __func__ << " Is YouTube, attempting to extract transcript...";
    rfh->ExecuteJavaScriptForTests(kYoutubeTranscriptExtractionScript, base::BindOnce([](AIChatTabHelper* instance, base::Value result){
      DVLOG(2) << "result is: " << base::Value::GetTypeName(result.type());
      auto* transcript_url_ref = result.GetIfString();
      if (!transcript_url_ref || transcript_url_ref->empty()) {
        // TODO(petemill): centralise handle no summary available
        DVLOG(2) << "result was empty";
        for (auto& obs : instance->observers_) {
          obs.OnRequestSummaryFailed();
        }
        return;
      }
      GURL transcript_url = GURL(*transcript_url_ref);
      DVLOG(2) << "result was url: " << transcript_url.spec();
      if (!transcript_url.is_valid()) {
        // TODO(petemill): centralise handle no summary available
        DVLOG(2) << "result was invalid.";
        for (auto& obs : instance->observers_) {
          obs.OnRequestSummaryFailed();
        }
        return;
      }
      auto request = std::make_unique<network::ResourceRequest>();
      request->url = GURL(transcript_url);
      request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES;
      request->credentials_mode = network::mojom::CredentialsMode::kOmit;
      request->method = net::HttpRequestHeaders::kGetMethod;
      auto url_loader = network::SimpleURLLoader::Create(
          std::move(request), GetNetworkTrafficAnnotationTag());
      url_loader->SetRetryOptions(
          1, network::SimpleURLLoader::RetryMode::RETRY_ON_5XX |
                network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);
      url_loader->SetAllowHttpErrorResults(true);
      auto iter = instance->url_loaders_.insert(instance->url_loaders_.begin(), std::move(url_loader));
      iter->get()->DownloadToString(
          instance->web_contents()->GetBrowserContext()
                                      ->GetDefaultStoragePartition()
                                      ->GetURLLoaderFactoryForBrowserProcess().get(),
          // Handle response
          base::BindOnce([](AIChatTabHelper* instance, SimpleURLLoaderList::iterator iter,
              std::unique_ptr<std::string> response_body) {
            auto* loader = iter->get();
            auto response_code = -1;
            base::flat_map<std::string, std::string> headers;
            if (loader->ResponseInfo()) {
              auto headers_list = loader->ResponseInfo()->headers;
              if (headers_list) {
                response_code = headers_list->response_code();
              }
            }
            instance->url_loaders_.erase(iter);
            // Validate if we get a feed
            std::string transcript_xml = response_body ? *response_body : "";
            if (response_code < 200 || response_code >= 300 || transcript_xml.empty()) {
              DVLOG(1) << __func__ << " invalid video transcript response from url: " << iter->get()->GetFinalURL().spec() << " status: " << response_code;
              return;
            }
            DVLOG(2) << "Got video text: " << transcript_xml;
            // Prevent indirect prompt injections being sent to the AI model.
            // TODO(petemill): Abstract prompt injection cleanups to a central place
            base::ReplaceSubstringsAfterOffset(&transcript_xml, 0, ai_chat::kHumanPrompt,
                                              "");
            base::ReplaceSubstringsAfterOffset(&transcript_xml, 0, ai_chat::kAIPrompt, "");

            VLOG(1) << __func__
                    << " Number of chars in video transcript xml = " << transcript_xml.length()
                    << "\n";

            std::string summarize_prompt = base::ReplaceStringPlaceholders(
                l10n_util::GetStringUTF8(IDS_AI_CHAT_SUMMARIZE_VIDEO_PROMPT), {transcript_xml},
                nullptr);

            // Get summary from API
            instance->MakeAPIRequestWithConversationHistoryUpdate(
                {CharacterType::HUMAN, ConversationTurnVisibility::HIDDEN,
                summarize_prompt});
          }, base::Unretained(instance),
          iter),
          2 * 1024 * 1024);
    }, this));
    return;
  }

  DVLOG(3) << __func__ << " URL host to get summary for is: " << web_contents()->GetLastCommittedURL().host();

  if (!rfh) {
    // TODO(petemill): Don't allow the UI to submit requests at this state
    VLOG(1) << "Summary request submitted for a WebContents without a"
               "primary AXTree-associated RenderFrameHost yet";
    for (auto& obs : observers_) {
      obs.OnRequestSummaryFailed();
    }
    return;
  }

  // TODO(@nullhook): Add a timeout and test this on real pages
  web_contents()->RequestAXTreeSnapshot(
      base::BindOnce(&AIChatTabHelper::OnSnapshotFinished,
                     base::Unretained(this)),
      ui::AXMode::kWebContents,
      /* max_nodes= */ 5000,
      /* timeout= */ {});
}

void AIChatTabHelper::OnSnapshotFinished(const ui::AXTreeUpdate& snapshot) {
  ui::AXTree tree;
  if (!tree.Unserialize(snapshot)) {
    for (auto& obs : observers_) {
      obs.OnRequestSummaryFailed();
    }
    return;
  }

  // Start AX distillation process
  // Don't copy the tree, as it can be expensive.
  DistillViaAlgorithm(tree);
}

void AIChatTabHelper::DistillViaAlgorithm(const ui::AXTree& tree) {
  std::vector<const ui::AXNode*> content_root_nodes;
  std::vector<const ui::AXNode*> content_nodes;
  GetContentRootNodes(tree.root(), &content_root_nodes);

  for (const ui::AXNode* content_root_node : content_root_nodes) {
    AddContentNodesToVector(content_root_node, &content_nodes);
  }

  std::vector<std::u16string> text_node_contents;
  for (const ui::AXNode* content_node : content_nodes) {
    AddTextNodesToVector(content_node, &text_node_contents);
  }

  // TODO(nullhook): The assumption here is that 9300 chars equate to
  // approximately 2k tokens, which is a rough estimate. A proper tokenizer is
  // needed for accurate measurement.
  std::string contents_text = base::UTF16ToUTF8(
      base::JoinString(text_node_contents, u" ").substr(0, 9300));
  if (contents_text.empty()) {
    VLOG(1) << __func__ << " Contents is empty\n";

    for (auto& obs : observers_) {
      obs.OnRequestSummaryFailed();
    }
    return;
  }

  // Prevent indirect prompt injections being sent to the AI model.
  // TODO(nullhook): Abstract prompt injection cleanups to a central place
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, ai_chat::kHumanPrompt,
                                     "");
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, ai_chat::kAIPrompt, "");
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, "<article>", "");
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, "</article>", "");
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, "<history>", "");
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, "</history>", "");
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, "<question>", "");
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, "</question>", "");

  VLOG(1) << __func__
          << " Number of chars in content text = " << contents_text.length()
          << "\n";

  article_text_ = contents_text;

  std::string summarize_prompt = "Summarize the above article.";

  // We hide the prompt with article content from the user
  MakeAPIRequestWithConversationHistoryUpdate(
      {CharacterType::HUMAN, ConversationTurnVisibility::INTERNAL,
       summarize_prompt});
}

void AIChatTabHelper::CleanUp() {
  chat_history_.clear();
  article_summary_.clear();
  article_text_.clear();
  is_request_in_progress_ = false;

  // Trigger an observer update to refresh the UI.
  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
  }
}

void AIChatTabHelper::MakeAPIRequestWithConversationHistoryUpdate(
    const ConversationTurn& turn) {
  std::string prompt = base::ReplaceStringPlaceholders(
      l10n_util::GetStringUTF8(IDS_AI_CHAT_SUMMARIZE_PROMPT),
      {article_text_, GetConversationHistoryString(), turn.text}, nullptr);

  std::string prompt_with_history =
      base::StrCat({prompt, ai_chat::kAIPrompt, " <response>\n"});

  if (turn.visibility != ConversationTurnVisibility::INTERNAL) {
    AddToConversationHistory(turn);
  }

  DCHECK(ai_chat_api_);

  // Assuming a hidden conversation has a summary prompt,
  // the incoming response is expected to include the AI-generated summary.
  // TODO(nullhook): Improve this heuristic, as it may or may not be true.
  bool is_summarize_prompt =
      turn.visibility == ConversationTurnVisibility::INTERNAL ? true : false;

  auto data_received_callback = base::BindRepeating(
      &AIChatTabHelper::OnAPIStreamDataReceived, base::Unretained(this));

  auto data_completed_callback =
      base::BindOnce(&AIChatTabHelper::OnAPIStreamDataComplete,
                     base::Unretained(this), is_summarize_prompt);

  is_request_in_progress_ = true;
  ai_chat_api_->QueryPrompt(std::move(prompt_with_history),
                            std::move(data_completed_callback),
                            std::move(data_received_callback));
}

bool AIChatTabHelper::IsRequestInProgress() {
  DCHECK(ai_chat_api_);

  return is_request_in_progress_;
}

void AIChatTabHelper::OnAPIStreamDataReceived(
    data_decoder::DataDecoder::ValueOrError result) {
  if (!result.has_value() || !result->is_dict()) {
    return;
  }

  if (const std::string* completion =
          result->GetDict().FindString("completion")) {
    UpdateOrCreateLastAssistantEntry(*completion);

    // Trigger an observer update to refresh the UI.
    for (auto& obs : observers_) {
      obs.OnAPIRequestInProgress(IsRequestInProgress());
    }
  }
}

void AIChatTabHelper::OnAPIStreamDataComplete(
    bool is_summarize_prompt,
    api_request_helper::APIRequestResult result) {
  const bool success = result.Is2XXResponseCode();
  if (success) {
    // TODO(nullhook): Remove this as we don't cache summaries anymore
    if (is_summarize_prompt && !chat_history_.empty()) {
      const ConversationTurn& last_turn = chat_history_.back();
      if (last_turn.character_type == CharacterType::ASSISTANT) {
        SetArticleSummaryString(last_turn.text);
      }
    }

    // We're checking for a value body in case for non-streaming API results.
    if (result.value_body().is_dict()) {
      if (const std::string* completion =
              result.value_body().GetDict().FindString("completion")) {
        AddToConversationHistory(
            ConversationTurn{CharacterType::ASSISTANT,
                             ConversationTurnVisibility::VISIBLE, *completion});
      }
    }
  }

  if (!success) {
    // TODO(petemill): show error state separate from assistant message
    AddToConversationHistory(ConversationTurn{
        CharacterType::ASSISTANT, ConversationTurnVisibility::VISIBLE,
        l10n_util::GetStringUTF8(IDS_CHAT_UI_API_ERROR)});
  }

  is_request_in_progress_ = false;

  // Trigger an observer update to refresh the UI.
  for (auto& obs : observers_) {
    obs.OnAPIRequestInProgress(IsRequestInProgress());
  }
}

void AIChatTabHelper::PrimaryPageChanged(content::Page& page) {
  // TODO(nullhook): Cancel inflight API request
  CleanUp();
}

void AIChatTabHelper::WebContentsDestroyed() {
  CleanUp();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(AIChatTabHelper);
