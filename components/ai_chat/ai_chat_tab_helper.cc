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
#include "base/ranges/algorithm.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/ai_chat/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/constants.h"
#include "brave/components/ai_chat/pref_names.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
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
      content::WebContentsUserData<AIChatTabHelper>(*web_contents),
      pref_service_(
          *user_prefs::UserPrefs::Get(web_contents->GetBrowserContext())) {
  ai_chat_api_ =
      std::make_unique<AIChatAPI>(web_contents->GetBrowserContext()
                                      ->GetDefaultStoragePartition()
                                      ->GetURLLoaderFactoryForBrowserProcess());
  // TODO(petemill): Observe opt-in - don't send any requests before opt-in
}

AIChatTabHelper::~AIChatTabHelper() = default;

const std::vector<ConversationTurn>& AIChatTabHelper::GetConversationHistory() {
  return chat_history_;
}

bool AIChatTabHelper::HasUserOptedIn() {
  return pref_service_->GetBoolean(ai_chat::prefs::kBraveChatHasSeenDisclaimer);
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

void AIChatTabHelper::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void AIChatTabHelper::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void AIChatTabHelper::GeneratePageText() {
  auto* primary_rfh = web_contents()->GetPrimaryMainFrame();

  if (!primary_rfh) {
    VLOG(1) << "Summary request submitted for a WebContents without a "
               "primary main frame";
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
    VLOG(1) << "Summary request submitted for a WebContents without a"
               "primary AXTree-associated RenderFrameHost yet";
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
  // TODO(petemill): Check that the page hasn't navigated away, either
  // by checking navigation ID or refactoring this to a per-navigation
  // class instance.
  ui::AXTree tree;
  if (!tree.Unserialize(snapshot)) {
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

  // Now that we have article text, we can suggest to summarize it
  DCHECK(suggested_questions_.empty())
      << "Expected suggested questions to be clear when there has been no"
      << " previous text content but there were " << suggested_questions_.size()
      << " suggested questions: "
      << base::JoinString(suggested_questions_, ", ");
  suggested_questions_.emplace_back("Summarize this page");
  OnSuggestedQuestionsChanged();
  // Automatically fetch questions related to page content, if allowed
  bool can_auto_fetch_questions =
      HasUserOptedIn() &&
      pref_service_->GetBoolean(
          ai_chat::prefs::kBraveChatAutoGenerateQuestions) &&
      !article_text_.empty() && (suggested_questions_.size() <= 1);
  if (can_auto_fetch_questions) {
    GenerateQuestions();
  }
}

void AIChatTabHelper::CleanUp() {
  chat_history_.clear();
  article_text_.clear();
  suggested_questions_.clear();
  is_request_in_progress_ = false;
  has_generated_questions_ = false;
  OnSuggestedQuestionsChanged();


  // Trigger an observer update to refresh the UI.
  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
  }
}

std::vector<std::string> AIChatTabHelper::GetSuggestedQuestions(
    bool& can_generate,
    bool& auto_generate) {
  // Can we get suggested questions
  can_generate = !has_generated_questions_ && !article_text_.empty();
  // Are we allowed to auto-generate
  auto_generate = pref_service_->GetBoolean(
      ai_chat::prefs::kBraveChatAutoGenerateQuestions);
  return suggested_questions_;
}

void AIChatTabHelper::GenerateQuestions() {
  DVLOG(1) << __func__;
  // This function should not be presented in the UI if the user has not
  // opted-in yet.
  DCHECK(HasUserOptedIn());
  // Can't operate if we don't have an article text
  if (article_text_.empty()) {
    return;
  }
  // Don't perform the operation more than once
  if (suggested_questions_.size() > 1u) {
    return;
  }

  has_generated_questions_ = true;
  OnSuggestedQuestionsChanged();

  DCHECK(!is_request_in_progress_);
  std::string prompt = base::StrCat(
      {base::ReplaceStringPlaceholders(
           l10n_util::GetStringUTF8(IDS_AI_CHAT_ARTICLE_PROMPT_SEGMENT),
           {article_text_}, nullptr),
       "\n\n", l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_PROMPT_SEGMENT),
       ai_chat::kAIPrompt, " <response>"});
  // Make API request for questions.
  // Do not call SetRequestInProgress, this progress
  // does not need to be shown to the UI.
  auto navigation_id_for_query = current_navigation_id_;
  ai_chat_api_->QueryPrompt(prompt,
      base::BindOnce(
          [](AIChatTabHelper* tab_helper, int64_t navigation_id_for_query,
             int64_t navigation_id_for_query,
             api_request_helper::APIRequestResult result,
             bool success) {
            if (!tab_helper) {
              VLOG(1) << "TabHelper was deleted before API call completed";
              return;
            }
            auto success = result.Is2XXResponseCode();
            if (!success) {
              LOG(ERROR) << "Error getting question suggestions. Code: "
                         << result.response_code();
              return;
            }
            // Validate
            if (!result.value_body().is_dict()) {
              DVLOG(1) << "Expected dictionary for question suggestion result"
                      << " but got: "
                      << result.value_body().DebugString();
              return;
            }
            const std::string* completion =
                result.value_body().FindStringKey("completion");
            if (!completion || completion->empty()) {
              DVLOG(1) << "Expected completion param for question suggestion"
                      << " result but got: "
                      << result.value_body().DebugString();
              return;
            }

            DVLOG(2) << "Received " << (success ? "success" : "failed")
                    << " suggested questions response: " << completion;
            // We might have navigated away whilst this async operation is in
            // progress, so check if we're the same navigation.
            if (tab_helper->current_navigation_id_ != navigation_id_for_query) {
              VLOG(1) << "Navigation id was different: "
                      << tab_helper->current_navigation_id_ << " "
                      << navigation_id_for_query;
              return;
            }
            // Parse questions
            auto questions = base::SplitString(
                *completion, "|", base::WhitespaceHandling::TRIM_WHITESPACE,
                base::SplitResult::SPLIT_WANT_NONEMPTY);
            for (auto question : questions) {
              tab_helper->suggested_questions_.emplace_back(question);
            }
            // Notify observers
            tab_helper->OnSuggestedQuestionsChanged();
            DVLOG(2) << "Got questions:"
                    << base::JoinString(tab_helper->suggested_questions_, "\n");
          },
          weak_ptr_factory_.GetWeakPtr(), std::move(navigation_id_for_query)));
}

void AIChatTabHelper::MakeAPIRequestWithConversationHistoryUpdate(
    const ConversationTurn& turn) {
  // This function should not be presented in the UI if the user has not
  // opted-in yet.
  DCHECK(HasUserOptedIn());
  // If it's a suggested question, remove it
  if (turn.character_type == CharacterType::HUMAN) {
    auto found_question_iter =
        base::ranges::find(suggested_questions_, turn.text);
    if (found_question_iter != suggested_questions_.end()) {
      suggested_questions_.erase(found_question_iter);
      OnSuggestedQuestionsChanged();
    }
  }

  auto prompt_segment_article =
      article_text_.empty()
          ? ""
          : base::StrCat({base::ReplaceStringPlaceholders(
                              l10n_util::GetStringUTF8(
                                  IDS_AI_CHAT_ARTICLE_PROMPT_SEGMENT),
                              {article_text_}, nullptr),
                          "\n\n"});

  auto prompt_segment_history =
      chat_history_.empty()
          ? ""
          : base::ReplaceStringPlaceholders(
                l10n_util::GetStringUTF8(
                    IDS_AI_CHAT_ASSISTANT_HISTORY_PROMPT_SEGMENT),
                {GetConversationHistoryString()}, nullptr);

  std::string prompt = base::StrCat(
      {prompt_segment_article,
       base::ReplaceStringPlaceholders(
           l10n_util::GetStringUTF8(IDS_AI_CHAT_ASSISTANT_PROMPT_SEGMENT),
           {prompt_segment_history, turn.text}, nullptr),
       ai_chat::kAIPrompt, " <response>\n"});

  if (turn.visibility != ConversationTurnVisibility::INTERNAL) {
    AddToConversationHistory(turn);
  }

  DCHECK(ai_chat_api_);

  auto data_received_callback = base::BindRepeating(
      &AIChatTabHelper::OnAPIStreamDataReceived, base::Unretained(this));

  auto data_completed_callback =
      base::BindOnce(&AIChatTabHelper::OnAPIStreamDataComplete,
                     base::Unretained(this));

  is_request_in_progress_ = true;
  ai_chat_api_->QueryPrompt(std::move(prompt),
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
    api_request_helper::APIRequestResult result) {
  const bool success = result.Is2XXResponseCode();
  if (success) {
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

void AIChatTabHelper::OnSuggestedQuestionsChanged() {
  auto auto_generate = pref_service_->GetBoolean(
      ai_chat::prefs::kBraveChatAutoGenerateQuestions);
  for (auto& obs : observers_) {
    obs.OnSuggestedQuestionsChanged(suggested_questions_,
                                    has_generated_questions_, auto_generate);
  }
}

void AIChatTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }
  DVLOG(2) << __func__ << navigation_handle->GetNavigationId()
           << " url: " << navigation_handle->GetURL().spec();
  current_navigation_id_ = navigation_handle->GetNavigationId();
}

void AIChatTabHelper::PrimaryPageChanged(content::Page& page) {
  // TODO(nullhook): Cancel inflight API request
  CleanUp();
}

void AIChatTabHelper::DocumentOnLoadCompletedInPrimaryMainFrame() {
  // We might have content here, so check.
  // TODO(petemill): If there are other navigation events to also
  // check if content is available at, then start a queue and make
  // sure we don't have multiple async distills going on at the same time.
  GeneratePageText();
}

void AIChatTabHelper::WebContentsDestroyed() {
  CleanUp();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(AIChatTabHelper);
