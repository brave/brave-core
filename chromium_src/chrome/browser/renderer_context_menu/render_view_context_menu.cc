/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/renderer_context_menu/render_view_context_menu.h"

#include <optional>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/fixed_flat_set.h"
#include "base/feature_list.h"
#include "base/strings/string_util.h"
#include "brave/browser/autocomplete/brave_autocomplete_scheme_classifier.h"
#include "brave/browser/brave_shields/brave_shields_tab_helper.h"
#include "brave/browser/cosmetic_filters/cosmetic_filters_tab_helper.h"
#include "brave/browser/renderer_context_menu/brave_spelling_options_submenu_observer.h"
#include "brave/browser/ui/brave_pages.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/browser_dialogs.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/components/ai_rewriter/common/buildflags/buildflags.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_provider_client.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/common/channel_info.h"
#include "components/omnibox/browser/autocomplete_classifier.h"
#include "components/omnibox/browser/autocomplete_controller.h"
#include "components/omnibox/browser/autocomplete_match_type.h"
#include "content/public/browser/web_contents.h"
#include "net/base/filename_util.h"
#include "ui/base/models/menu_separator_types.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/paint_vector_icon.h"
#include "url/origin.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#endif

#include "base/feature_list.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "components/grit/brave_components_strings.h"

#if BUILDFLAG(ENABLE_AI_REWRITER)
#include "brave/browser/ui/ai_rewriter/ai_rewriter_dialog_delegate.h"
#include "brave/components/ai_rewriter/common/features.h"
#endif

// Our .h file creates a masquerade for RenderViewContextMenu.  Switch
// back to the Chromium one for the Chromium implementation.
#undef RenderViewContextMenu
#define RenderViewContextMenu RenderViewContextMenu_Chromium

namespace {

AutocompleteMatch GetAutocompleteMatchForText(Profile* profile,
                                              const std::u16string& text) {
  AutocompleteMatch match;
  AutocompleteClassifier classifier(
      std::make_unique<AutocompleteController>(
          std::make_unique<ChromeAutocompleteProviderClient>(profile),
          AutocompleteClassifier::DefaultOmniboxProviders()),
      std::make_unique<BraveAutocompleteSchemeClassifier>(profile));
  classifier.Classify(text, false, false,
                      metrics::OmniboxEventProto::INVALID_SPEC, &match, NULL);
  classifier.Shutdown();
  return match;
}

GURL GetSelectionNavigationURL(Profile* profile, const std::u16string& text) {
  return GetAutocompleteMatchForText(profile, text).destination_url;
}

std::optional<GURL> GetSelectedURL(Profile* profile,
                                   const std::u16string& text) {
  auto match = GetAutocompleteMatchForText(profile, text);
  if (match.type != AutocompleteMatchType::URL_WHAT_YOU_TYPED) {
    return std::nullopt;
  }
  return match.destination_url;
}

base::OnceCallback<void(BraveRenderViewContextMenu*)>*
BraveGetMenuShownCallback() {
  static base::NoDestructor<
      base::OnceCallback<void(BraveRenderViewContextMenu*)>>
      callback;
  return callback.get();
}

}  // namespace

void RenderViewContextMenu::RegisterMenuShownCallbackForTesting(
    base::OnceCallback<void(BraveRenderViewContextMenu*)> cb) {
  *BraveGetMenuShownCallback() = std::move(cb);
}

#define BRAVE_APPEND_SEARCH_PROVIDER                                     \
  if (GetProfile()->IsOffTheRecord()) {                                  \
    selection_navigation_url_ =                                          \
        GetSelectionNavigationURL(GetProfile(), params_.selection_text); \
    if (!selection_navigation_url_.is_valid())                           \
      return;                                                            \
  }

// Use our subclass to initialize SpellingOptionsSubMenuObserver.
#define SpellingOptionsSubMenuObserver BraveSpellingOptionsSubMenuObserver
#define RegisterMenuShownCallbackForTesting \
  RegisterMenuShownCallbackForTesting_unused

#include "src/chrome/browser/renderer_context_menu/render_view_context_menu.cc"

#undef SpellingOptionsSubMenuObserver
#undef RegisterMenuShownCallbackForTesting

// Make it clear which class we mean here.
#undef RenderViewContextMenu
#undef BRAVE_APPEND_SEARCH_PROVIDER

namespace {

#if BUILDFLAG(ENABLE_TOR)
bool HasAlreadyOpenedTorWindow(Profile* profile) {
  for (Browser* browser : *BrowserList::GetInstance()) {
    if (browser->profile()->IsTor() &&
        browser->profile()->GetOriginalProfile() == profile) {
      return true;
    }
  }

  return false;
}

// Modified OnProfileCreated() in render_view_context_menu.cc
// to handle additional |use_new_tab| param.
void OnTorProfileCreated(const GURL& link_url,
                         bool use_new_tab,
                         Browser* browser) {
  CHECK(browser);
  /* |ui::PAGE_TRANSITION_TYPED| is used rather than
     |ui::PAGE_TRANSITION_LINK| since this ultimately opens the link in
     another browser. This parameter is used within the tab strip model of
     the browser it opens in implying a link from the active tab in the
     destination browser which is not correct. */
  NavigateParams nav_params(browser, link_url, ui::PAGE_TRANSITION_TYPED);
  if (use_new_tab) {
    nav_params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  } else {
    // Stop current loading to show tab throbber wait spinning till tor is
    // initialized.
    if (auto* contents = browser->tab_strip_model()->GetActiveWebContents()) {
      contents->Stop();
      nav_params.disposition = WindowOpenDisposition::CURRENT_TAB;
    }
  }
  nav_params.referrer =
      content::Referrer(GURL(), network::mojom::ReferrerPolicy::kStrictOrigin);
  nav_params.window_action = NavigateParams::SHOW_WINDOW;
  Navigate(&nav_params);
}

#endif

#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
void OnGetImageForTextCopy(base::WeakPtr<content::WebContents> web_contents,
                           const SkBitmap& image) {
  if (!web_contents) {
    return;
  }

  brave::ShowTextRecognitionDialog(web_contents.get(), image);
}
#endif

constexpr char kAIChatRewriteDataKey[] = "ai_chat_rewrite_data";

struct AIChatRewriteData : public base::SupportsUserData::Data {
  bool has_data_received = false;
};

bool IsRewriteCommand(int command) {
  static constexpr auto kRewriteCommands = base::MakeFixedFlatSet<int>(
      {IDC_AI_CHAT_CONTEXT_PARAPHRASE, IDC_AI_CHAT_CONTEXT_IMPROVE,
       IDC_AI_CHAT_CONTEXT_ACADEMICIZE, IDC_AI_CHAT_CONTEXT_PROFESSIONALIZE,
       IDC_AI_CHAT_CONTEXT_PERSUASIVE_TONE, IDC_AI_CHAT_CONTEXT_CASUALIZE,
       IDC_AI_CHAT_CONTEXT_FUNNY_TONE, IDC_AI_CHAT_CONTEXT_SHORTEN,
       IDC_AI_CHAT_CONTEXT_EXPAND});

  return kRewriteCommands.contains(command);
}

std::pair<ai_chat::mojom::ActionType, ai_chat::ContextMenuAction>
GetActionTypeAndP3A(int command) {
  static constexpr auto kActionTypeMap = base::MakeFixedFlatMap<
      int, std::pair<ai_chat::mojom::ActionType, ai_chat::ContextMenuAction>>(
      {{IDC_AI_CHAT_CONTEXT_SUMMARIZE_TEXT,
        {ai_chat::mojom::ActionType::SUMMARIZE_SELECTED_TEXT,
         ai_chat::ContextMenuAction::kSummarize}},
       {IDC_AI_CHAT_CONTEXT_EXPLAIN,
        {ai_chat::mojom::ActionType::EXPLAIN,
         ai_chat::ContextMenuAction::kExplain}},
       {IDC_AI_CHAT_CONTEXT_PARAPHRASE,
        {ai_chat::mojom::ActionType::PARAPHRASE,
         ai_chat::ContextMenuAction::kParaphrase}},
       {IDC_AI_CHAT_CONTEXT_CREATE_TAGLINE,
        {ai_chat::mojom::ActionType::CREATE_TAGLINE,
         ai_chat::ContextMenuAction::kCreateTagline}},
       {IDC_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_COMMENT_SHORT,
        {ai_chat::mojom::ActionType::CREATE_SOCIAL_MEDIA_COMMENT_SHORT,
         ai_chat::ContextMenuAction::kCreateSocialMedia}},
       {IDC_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_COMMENT_LONG,
        {ai_chat::mojom::ActionType::CREATE_SOCIAL_MEDIA_COMMENT_LONG,
         ai_chat::ContextMenuAction::kCreateSocialMedia}},
       {IDC_AI_CHAT_CONTEXT_IMPROVE,
        {ai_chat::mojom::ActionType::IMPROVE,
         ai_chat::ContextMenuAction::kImprove}},
       {IDC_AI_CHAT_CONTEXT_ACADEMICIZE,
        {ai_chat::mojom::ActionType::ACADEMICIZE,
         ai_chat::ContextMenuAction::kChangeTone}},
       {IDC_AI_CHAT_CONTEXT_PROFESSIONALIZE,
        {ai_chat::mojom::ActionType::PROFESSIONALIZE,
         ai_chat::ContextMenuAction::kChangeTone}},
       {IDC_AI_CHAT_CONTEXT_PERSUASIVE_TONE,
        {ai_chat::mojom::ActionType::PERSUASIVE_TONE,
         ai_chat::ContextMenuAction::kChangeTone}},
       {IDC_AI_CHAT_CONTEXT_CASUALIZE,
        {ai_chat::mojom::ActionType::CASUALIZE,
         ai_chat::ContextMenuAction::kChangeTone}},
       {IDC_AI_CHAT_CONTEXT_FUNNY_TONE,
        {ai_chat::mojom::ActionType::FUNNY_TONE,
         ai_chat::ContextMenuAction::kChangeTone}},
       {IDC_AI_CHAT_CONTEXT_SHORTEN,
        {ai_chat::mojom::ActionType::SHORTEN,
         ai_chat::ContextMenuAction::kChangeLength}},
       {IDC_AI_CHAT_CONTEXT_EXPAND,
        {ai_chat::mojom::ActionType::EXPAND,
         ai_chat::ContextMenuAction::kChangeLength}}});
  CHECK(kActionTypeMap.contains(command));
  return kActionTypeMap.at(command);
}

void OnRewriteSuggestionDataReceived(
    base::WeakPtr<content::WebContents> web_contents,
    const std::string& suggestion) {
  if (!web_contents) {
    return;
  }

  auto* rewrite_data = static_cast<AIChatRewriteData*>(
      web_contents->GetUserData(kAIChatRewriteDataKey));
  if (!rewrite_data) {
    return;
  }

  if (rewrite_data->has_data_received) {
    // Subsequent data received, undo previous streaming result.
    web_contents->Undo();
  } else {
    rewrite_data->has_data_received = true;
  }

  web_contents->Replace(base::UTF8ToUTF16(suggestion));
}

void OnRewriteSuggestionCompleted(
    base::WeakPtr<content::WebContents> web_contents,
    const std::string& selected_text,
    ai_chat::mojom::ActionType action_type,
    base::expected<std::string, ai_chat::mojom::APIError> result) {
  if (!web_contents) {
    return;
  }

  if (!result.has_value()) {
    // If the content has been rewritten by previous streaming result, undo to
    // get back to original text.
    auto* rewrite_data = static_cast<AIChatRewriteData*>(
        web_contents->GetUserData(kAIChatRewriteDataKey));
    if (!rewrite_data) {
      return;
    }

    if (rewrite_data->has_data_received) {
      web_contents->Undo();
    }

    // Show the error in Leo side panel UI.
    Browser* browser = chrome::FindBrowserWithTab(web_contents.get());
    if (!browser) {
      return;
    }
    ai_chat::AIChatService* ai_chat_service =
        ai_chat::AIChatServiceFactory::GetForBrowserContext(
            web_contents.get()->GetBrowserContext());
    if (!ai_chat_service) {
      return;
    }
    ai_chat::AIChatTabHelper* helper =
        ai_chat::AIChatTabHelper::FromWebContents(web_contents.get());
    if (!helper) {
      return;
    }
    ai_chat::ConversationHandler* conversation =
        ai_chat_service->GetOrCreateConversationHandlerForContent(
            helper->GetContentId(), helper->GetWeakPtr());
    if (!conversation) {
      return;
    }
    conversation->MaybeUnlinkAssociatedContent();

    auto* sidebar_controller =
        static_cast<BraveBrowser*>(browser)->sidebar_controller();
    CHECK(sidebar_controller);
    sidebar_controller->ActivatePanelItem(
        sidebar::SidebarItem::BuiltInItemType::kChatUI);

    conversation->AddSubmitSelectedTextError(selected_text, action_type,
                                             result.error());
  }

  web_contents->RemoveUserData(kAIChatRewriteDataKey);
}

bool CanOpenSplitViewForWebContents(
    base::WeakPtr<content::WebContents> web_contents) {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveSplitView)) {
    return false;
  }

  if (!web_contents) {
    return false;
  }

  Browser* browser = chrome::FindBrowserWithTab(web_contents.get());
  return browser && browser->is_type_normal() &&
         brave::CanOpenNewSplitViewForTab(browser);
}

void OpenLinkInSplitView(base::WeakPtr<content::WebContents> web_contents,
                         const GURL& url) {
  if (!web_contents) {
    return;
  }

  Browser* browser = chrome::FindBrowserWithTab(web_contents.get());
  brave::NewSplitViewForTab(browser, std::nullopt, url);
}

}  // namespace

BraveRenderViewContextMenu::BraveRenderViewContextMenu(
    content::RenderFrameHost& render_frame_host,
    const content::ContextMenuParams& params)
    : RenderViewContextMenu_Chromium(render_frame_host, params),
      ai_chat_submenu_model_(this),
      ai_chat_change_tone_submenu_model_(this),
      ai_chat_change_length_submenu_model_(this),
      ai_chat_social_media_post_submenu_model_(this) {}

BraveRenderViewContextMenu::~BraveRenderViewContextMenu() = default;

bool BraveRenderViewContextMenu::IsCommandIdEnabled(int id) const {
  switch (id) {
#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
    case IDC_CONTENT_CONTEXT_COPY_TEXT_FROM_IMAGE:
      return params_.has_image_contents;
#endif
    case IDC_COPY_CLEAN_LINK:
      return params_.link_url.is_valid() ||
             GetSelectedURL(GetProfile(), params_.selection_text).has_value();
    case IDC_CONTENT_CONTEXT_FORCE_PASTE:
      // only enable if there is plain text data to paste - this is what
      // IsPasteAndMatchStyleEnabled checks internally, but IsPasteEnabled
      // allows non text types
      return IsPasteAndMatchStyleEnabled();
    case IDC_CONTENT_CONTEXT_OPENLINKTOR:
#if BUILDFLAG(ENABLE_TOR)
      if (TorProfileServiceFactory::IsTorDisabled(GetProfile())) {
        return false;
      }

      return params_.link_url.is_valid() &&
             IsURLAllowedInIncognito(params_.link_url) &&
             !GetProfile()->IsTor();
#else
      return false;
#endif
    case IDC_AI_CHAT_CONTEXT_SUMMARIZE_TEXT:
    case IDC_AI_CHAT_CONTEXT_LEO_TOOLS:
    case IDC_AI_CHAT_CONTEXT_EXPLAIN:
    case IDC_AI_CHAT_CONTEXT_PARAPHRASE:
    case IDC_AI_CHAT_CONTEXT_CREATE_TAGLINE:
    case IDC_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_COMMENT_SHORT:
    case IDC_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_COMMENT_LONG:
    case IDC_AI_CHAT_CONTEXT_IMPROVE:
    case IDC_AI_CHAT_CONTEXT_CHANGE_TONE:
    case IDC_AI_CHAT_CONTEXT_ACADEMICIZE:
    case IDC_AI_CHAT_CONTEXT_PROFESSIONALIZE:
    case IDC_AI_CHAT_CONTEXT_PERSUASIVE_TONE:
    case IDC_AI_CHAT_CONTEXT_CASUALIZE:
    case IDC_AI_CHAT_CONTEXT_FUNNY_TONE:
    case IDC_AI_CHAT_CONTEXT_SHORTEN:
    case IDC_AI_CHAT_CONTEXT_EXPAND:
    case IDC_AI_CHAT_CONTEXT_CHANGE_LENGTH:
    case IDC_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_POST:
      return IsAIChatEnabled();
#if BUILDFLAG(ENABLE_AI_REWRITER)
    case IDC_AI_CHAT_CONTEXT_REWRITE:
      return ai_rewriter::features::IsAIRewriterEnabled();
#endif
    case IDC_CONTENT_CONTEXT_OPENLINK_SPLIT_VIEW:
      return CanOpenSplitViewForWebContents(source_web_contents_->GetWeakPtr());
    case IDC_ADBLOCK_CONTEXT_BLOCK_ELEMENTS:
      return true;
    default:
      return RenderViewContextMenu_Chromium::IsCommandIdEnabled(id);
  }
}

void BraveRenderViewContextMenu::ExecuteCommand(int id, int event_flags) {
  switch (id) {
    case IDC_COPY_CLEAN_LINK: {
      GURL link_url = params_.link_url;
      if (!link_url.is_valid()) {
        auto selected_url =
            GetSelectedURL(GetProfile(), params_.selection_text);
        if (selected_url.has_value()) {
          link_url = selected_url.value();
        } else {
          return;
        }
      }
      brave::CopyLinkWithStrictCleaning(GetBrowser(), link_url);
    }; break;
    case IDC_CONTENT_CONTEXT_FORCE_PASTE: {
      std::u16string result;
      ui::Clipboard::GetForCurrentThread()->ReadText(
          ui::ClipboardBuffer::kCopyPaste,
          CreateDataEndpoint(/*notify_if_restricted=*/true).get(), &result);
      // Replace works just like Paste, but it doesn't trigger onpaste handlers
      source_web_contents_->Replace(result);
    }; break;
#if BUILDFLAG(ENABLE_TOR)
    case IDC_CONTENT_CONTEXT_OPENLINKTOR: {
      const bool has_tor_window = HasAlreadyOpenedTorWindow(GetProfile());
      Browser* tor_browser =
          TorProfileManager::SwitchToTorProfile(GetProfile());
      if (tor_browser) {
        OnTorProfileCreated(params_.link_url, has_tor_window, tor_browser);
      }
    } break;
#endif
#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
    case IDC_CONTENT_CONTEXT_COPY_TEXT_FROM_IMAGE:
      CopyTextFromImage();
      break;
#endif
    case IDC_AI_CHAT_CONTEXT_SUMMARIZE_TEXT:
    case IDC_AI_CHAT_CONTEXT_EXPLAIN:
    case IDC_AI_CHAT_CONTEXT_PARAPHRASE:
    case IDC_AI_CHAT_CONTEXT_CREATE_TAGLINE:
    case IDC_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_COMMENT_SHORT:
    case IDC_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_COMMENT_LONG:
    case IDC_AI_CHAT_CONTEXT_IMPROVE:
    case IDC_AI_CHAT_CONTEXT_ACADEMICIZE:
    case IDC_AI_CHAT_CONTEXT_PROFESSIONALIZE:
    case IDC_AI_CHAT_CONTEXT_PERSUASIVE_TONE:
    case IDC_AI_CHAT_CONTEXT_CASUALIZE:
    case IDC_AI_CHAT_CONTEXT_FUNNY_TONE:
    case IDC_AI_CHAT_CONTEXT_SHORTEN:
    case IDC_AI_CHAT_CONTEXT_EXPAND:
      ExecuteAIChatCommand(id);
      break;
#if BUILDFLAG(ENABLE_AI_REWRITER)
    case IDC_AI_CHAT_CONTEXT_REWRITE:
      ai_rewriter::AIRewriterDialogDelegate::Show(
          source_web_contents_, base::UTF16ToUTF8(params_.selection_text));
      break;
#endif
    case IDC_CONTENT_CONTEXT_OPENLINK_SPLIT_VIEW:
      OpenLinkInSplitView(source_web_contents_->GetWeakPtr(), params_.link_url);
      break;
    case IDC_ADBLOCK_CONTEXT_BLOCK_ELEMENTS:
      cosmetic_filters::CosmeticFiltersTabHelper::LaunchContentPicker(
          source_web_contents_);
      break;
    default:
      RenderViewContextMenu_Chromium::ExecuteCommand(id, event_flags);
  }
}

#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
void BraveRenderViewContextMenu::CopyTextFromImage() {
  RenderFrameHost* frame_host = GetRenderFrameHost();
  if (frame_host) {
    frame_host->GetImageAt(params_.x, params_.y,
                           base::BindOnce(OnGetImageForTextCopy,
                                          source_web_contents_->GetWeakPtr()));
  }
}
#endif

bool BraveRenderViewContextMenu::IsAIChatEnabled() const {
  return !params_.selection_text.empty() &&
         ai_chat::IsAIChatEnabled(GetProfile()->GetPrefs()) &&
         GetProfile()->IsRegularProfile() &&
         GetProfile()->GetPrefs()->GetBoolean(
             ai_chat::prefs::kBraveAIChatContextMenuEnabled) &&
         !IsInProgressiveWebApp();
}

void BraveRenderViewContextMenu::ExecuteAIChatCommand(int command) {
  // To do rewrite in-place, the following conditions must be met:
  // 1) Selected content is editable.
  // 2) User has opted in to Leo.
  // 3) Context menu rewrite in place feature is enabled.
  // 4) SSE is enabled, it is required otherwise the UI update will be too slow.
  // 5) The command is a rewrite command.
  // 6) There's no in-progress in-place rewrite.
  bool rewrite_in_place =
      params_.is_editable &&
      ai_chat::HasUserOptedIn(GetProfile()->GetPrefs()) &&
      ai_chat::features::IsContextMenuRewriteInPlaceEnabled() &&
      ai_chat::features::kAIChatSSE.Get() && IsRewriteCommand(command) &&
      !source_web_contents_->GetUserData(kAIChatRewriteDataKey);

  auto [action_type, p3a_action] = GetActionTypeAndP3A(command);
  auto selected_text = base::UTF16ToUTF8(params_.selection_text);

  if (rewrite_in_place) {
    source_web_contents_->SetUserData(kAIChatRewriteDataKey,
                                      std::make_unique<AIChatRewriteData>());
    if (!ai_engine_) {
      ai_engine_ = ai_chat::AIChatServiceFactory::GetForBrowserContext(
                       source_web_contents_->GetBrowserContext())
                       ->GetDefaultAIEngine();
    }
    ai_engine_->GenerateRewriteSuggestion(
        selected_text, ai_chat::GetActionTypeQuestion(action_type),
        /*selected_language*/ "",
        ai_chat::BindParseRewriteReceivedData(
            base::BindRepeating(&OnRewriteSuggestionDataReceived,
                                source_web_contents_->GetWeakPtr())),
        base::BindOnce(&OnRewriteSuggestionCompleted,
                       source_web_contents_->GetWeakPtr(), selected_text,
                       action_type));
  } else {
    auto* browser = GetBrowser();
    if (!browser) {
      VLOG(1) << "Can't get browser";
      return;
    }

    ai_chat::AIChatTabHelper* helper =
        ai_chat::AIChatTabHelper::FromWebContents(source_web_contents_);
    if (!helper) {
      VLOG(1) << "Can't get AI chat tab helper";
      return;
    }

    ai_chat::ConversationHandler* conversation =
        ai_chat::AIChatServiceFactory::GetForBrowserContext(
            source_web_contents_->GetBrowserContext())
            ->GetOrCreateConversationHandlerForContent(helper->GetContentId(),
                                                       helper->GetWeakPtr());
    // Before trying to activate the panel, unlink page content if needed.
    // This needs to be called before activating the panel to check against the
    // current state.
    conversation->MaybeUnlinkAssociatedContent();

    // Active the panel.
    auto* sidebar_controller =
        static_cast<BraveBrowser*>(browser)->sidebar_controller();
    CHECK(sidebar_controller);
    sidebar_controller->ActivatePanelItem(
        sidebar::SidebarItem::BuiltInItemType::kChatUI);
    conversation->SubmitSelectedText(selected_text, action_type);
  }

  g_brave_browser_process->process_misc_metrics()
      ->ai_chat_metrics()
      ->RecordContextMenuUsage(p3a_action);
}

void BraveRenderViewContextMenu::BuildAIChatMenu() {
  if (!IsAIChatEnabled()) {
    return;
  }
  std::optional<size_t> print_index =
      menu_model_.GetIndexOfCommandId(IDC_PRINT);
  if (!print_index.has_value()) {
    return;
  }

  ai_chat_submenu_model_.AddTitleWithStringId(
      IDS_AI_CHAT_CONTEXT_QUICK_ACTIONS);

#if BUILDFLAG(ENABLE_AI_REWRITER)
  if (ai_rewriter::features::IsAIRewriterEnabled()) {
    ai_chat_submenu_model_.AddItemWithStringId(IDC_AI_CHAT_CONTEXT_REWRITE,
                                               IDS_AI_CHAT_CONTEXT_REWRITE);
  }
#endif

  ai_chat_submenu_model_.AddItemWithStringId(
      IDC_AI_CHAT_CONTEXT_SUMMARIZE_TEXT, IDS_AI_CHAT_CONTEXT_SUMMARIZE_TEXT);
  ai_chat_submenu_model_.AddItemWithStringId(IDC_AI_CHAT_CONTEXT_EXPLAIN,
                                             IDS_AI_CHAT_CONTEXT_EXPLAIN);
  ai_chat_submenu_model_.AddSeparator(ui::NORMAL_SEPARATOR);

  ai_chat_submenu_model_.AddTitleWithStringId(IDS_AI_CHAT_CONTEXT_REWRITE);
  ai_chat_submenu_model_.AddItemWithStringId(IDC_AI_CHAT_CONTEXT_PARAPHRASE,
                                             IDS_AI_CHAT_CONTEXT_PARAPHRASE);
  ai_chat_submenu_model_.AddItemWithStringId(IDC_AI_CHAT_CONTEXT_IMPROVE,
                                             IDS_AI_CHAT_CONTEXT_IMPROVE);

  ai_chat_change_tone_submenu_model_.AddItemWithStringId(
      IDC_AI_CHAT_CONTEXT_ACADEMICIZE, IDS_AI_CHAT_CONTEXT_ACADEMICIZE);
  ai_chat_change_tone_submenu_model_.AddItemWithStringId(
      IDC_AI_CHAT_CONTEXT_PROFESSIONALIZE, IDS_AI_CHAT_CONTEXT_PROFESSIONALIZE);
  ai_chat_change_tone_submenu_model_.AddItemWithStringId(
      IDC_AI_CHAT_CONTEXT_PERSUASIVE_TONE, IDS_AI_CHAT_CONTEXT_PERSUASIVE_TONE);
  ai_chat_change_tone_submenu_model_.AddItemWithStringId(
      IDC_AI_CHAT_CONTEXT_CASUALIZE, IDS_AI_CHAT_CONTEXT_CASUALIZE);
  ai_chat_change_tone_submenu_model_.AddItemWithStringId(
      IDC_AI_CHAT_CONTEXT_FUNNY_TONE, IDS_AI_CHAT_CONTEXT_FUNNY_TONE);

  ai_chat_submenu_model_.AddSubMenuWithStringId(
      IDC_AI_CHAT_CONTEXT_CHANGE_TONE, IDS_AI_CHAT_CONTEXT_CHANGE_TONE,
      &ai_chat_change_tone_submenu_model_);

  ai_chat_change_length_submenu_model_.AddItemWithStringId(
      IDC_AI_CHAT_CONTEXT_SHORTEN, IDS_AI_CHAT_CONTEXT_SHORTEN);
  ai_chat_change_length_submenu_model_.AddItemWithStringId(
      IDC_AI_CHAT_CONTEXT_EXPAND, IDS_AI_CHAT_CONTEXT_EXPAND);
  ai_chat_submenu_model_.AddSubMenuWithStringId(
      IDC_AI_CHAT_CONTEXT_CHANGE_LENGTH, IDS_AI_CHAT_CONTEXT_CHANGE_LENGTH,
      &ai_chat_change_length_submenu_model_);

  ai_chat_submenu_model_.AddSeparator(ui::NORMAL_SEPARATOR);

  ai_chat_submenu_model_.AddTitleWithStringId(IDS_AI_CHAT_CONTEXT_CREATE);
  ai_chat_submenu_model_.AddItemWithStringId(
      IDC_AI_CHAT_CONTEXT_CREATE_TAGLINE, IDS_AI_CHAT_CONTEXT_CREATE_TAGLINE);

  ai_chat_social_media_post_submenu_model_.AddItemWithStringId(
      IDC_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_COMMENT_SHORT,
      IDS_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_COMMENT_SHORT);
  ai_chat_social_media_post_submenu_model_.AddItemWithStringId(
      IDC_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_COMMENT_LONG,
      IDS_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_COMMENT_LONG);
  ai_chat_submenu_model_.AddSubMenuWithStringId(
      IDC_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_POST,
      IDS_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_POST,
      &ai_chat_social_media_post_submenu_model_);

  menu_model_.InsertSubMenuWithStringIdAt(
      *print_index, IDC_AI_CHAT_CONTEXT_LEO_TOOLS,
      IDS_AI_CHAT_CONTEXT_LEO_TOOLS, &ai_chat_submenu_model_);
}

void BraveRenderViewContextMenu::AddSpellCheckServiceItem(bool is_checked) {
  // Call our implementation, not the one in the base class.
  // Assumption:
  // Use of spelling service is disabled in Brave profile preferences.
  DCHECK(!GetProfile()->GetPrefs()->GetBoolean(
      spellcheck::prefs::kSpellCheckUseSpellingService));
  AddSpellCheckServiceItem(&menu_model_, is_checked);
}

// static
void BraveRenderViewContextMenu::AddSpellCheckServiceItem(
    ui::SimpleMenuModel* menu,
    bool is_checked) {
  // Suppress adding "Spellcheck->Ask Brave for suggestions" item.
}

void BraveRenderViewContextMenu::AddAccessibilityLabelsServiceItem(
    bool is_checked) {
  // Suppress adding "Get image descriptions from Brave"
}

void BraveRenderViewContextMenu::AppendDeveloperItems() {
  RenderViewContextMenu_Chromium::AppendDeveloperItems();

  auto* shields_tab_helper =
      brave_shields::BraveShieldsTabHelper::FromWebContents(
          source_web_contents_);
  bool add_block_elements = shields_tab_helper &&
                            shields_tab_helper->GetBraveShieldsEnabled() &&
                            shields_tab_helper->GetAdBlockMode() !=
                                brave_shields::mojom::AdBlockMode::ALLOW;
#if BUILDFLAG(IS_ANDROID)
  // Content picker doesn't available for Android.
  add_block_elements = false;
#endif  // BUILDFLAG(IS_ANDROID)
  add_block_elements &=
      params_.selection_text.empty() || !params_.link_url.is_empty();

  const auto page_url = source_web_contents_->GetLastCommittedURL();
  add_block_elements &= page_url.SchemeIsHTTPOrHTTPS();
  add_block_elements &= base::FeatureList::IsEnabled(
      brave_shields::features::kBraveShieldsElementPicker);
  if (add_block_elements) {
    std::optional<size_t> inspect_index =
        menu_model_.GetIndexOfCommandId(IDC_CONTENT_CONTEXT_INSPECTELEMENT);
    if (inspect_index) {
      menu_model_.InsertItemWithStringIdAt(*inspect_index,
                                           IDC_ADBLOCK_CONTEXT_BLOCK_ELEMENTS,
                                           IDS_ADBLOCK_CONTEXT_BLOCK_ELEMENTS);
    } else {
      menu_model_.AddItemWithStringId(IDC_ADBLOCK_CONTEXT_BLOCK_ELEMENTS,
                                      IDS_ADBLOCK_CONTEXT_BLOCK_ELEMENTS);
    }
  }
}

void BraveRenderViewContextMenu::SetAIEngineForTesting(
    std::unique_ptr<ai_chat::EngineConsumer> ai_engine) {
  ai_engine_ = std::move(ai_engine);
}

void BraveRenderViewContextMenu::InitMenu() {
  RenderViewContextMenu_Chromium::InitMenu();

  std::optional<size_t> index = menu_model_.GetIndexOfCommandId(
      IDC_CONTENT_CONTEXT_PASTE_AND_MATCH_STYLE);
  if (index.has_value()) {
    menu_model_.InsertItemWithStringIdAt(index.value() + 1,
                                         IDC_CONTENT_CONTEXT_FORCE_PASTE,
                                         IDS_CONTENT_CONTEXT_FORCE_PASTE);
  }
#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
  const bool media_image = content_type_->SupportsGroup(
      ContextMenuContentType::ITEM_GROUP_MEDIA_IMAGE);
  if (media_image) {
    index =
        menu_model_.GetIndexOfCommandId(IDC_CONTENT_CONTEXT_COPYIMAGELOCATION);
    DCHECK(index);
    menu_model_.InsertItemWithStringIdAt(
        index.value() + 1, IDC_CONTENT_CONTEXT_COPY_TEXT_FROM_IMAGE,
        IDS_CONTENT_CONTEXT_COPY_TEXT_FROM_IMAGE);
  }
#endif

#if BUILDFLAG(ENABLE_TOR)
  // Add Open Link with Tor
  if (!TorProfileServiceFactory::IsTorDisabled(GetProfile()) &&
      content_type_->SupportsGroup(ContextMenuContentType::ITEM_GROUP_LINK) &&
      !params_.link_url.is_empty() && !params_.link_url.spec().starts_with(kAIChatUIURL)) {
    const Browser* browser = GetBrowser();
    const bool is_app = browser && browser->is_type_app();

    index = menu_model_.GetIndexOfCommandId(
        IDC_CONTENT_CONTEXT_OPENLINKOFFTHERECORD);
    DCHECK(index.has_value());

    menu_model_.InsertItemWithStringIdAt(
        index.value() + 1, IDC_CONTENT_CONTEXT_OPENLINKTOR,
        is_app ? IDS_CONTENT_CONTEXT_OPENLINKTOR_INAPP
               : IDS_CONTENT_CONTEXT_OPENLINKTOR);
  }
#endif

  if (params_.link_url.spec().starts_with(kAIChatUIURL) && GetProfile()->
      GetPrefs()->GetBoolean(ai_chat::prefs::kBraveAIChatContextMenuEnabled)) {
    index = menu_model_->.GetIndexOfCommandId(IDC_CONTENT_CONTEXT_OPENLINKOFFTHERECORD);
    DCHECK(index);
    menu_model_.RemoveItemAt(index.value());
  }

  if (!params_.link_url.is_empty() && params_.link_url.SchemeIsHTTPOrHTTPS()) {
    std::optional<size_t> link_index =
        menu_model_.GetIndexOfCommandId(IDC_CONTENT_CONTEXT_COPYLINKLOCATION);
    if (link_index.has_value()) {
      menu_model_.InsertItemWithStringIdAt(
          link_index.value() + 1, IDC_COPY_CLEAN_LINK, IDS_COPY_CLEAN_LINK);
    }
  }
  if (GetSelectedURL(GetProfile(), params_.selection_text).has_value()) {
    std::optional<size_t> copy_index =
        menu_model_.GetIndexOfCommandId(IDC_CONTENT_CONTEXT_COPY);
    if (copy_index.has_value() &&
        !menu_model_.GetIndexOfCommandId(IDC_COPY_CLEAN_LINK).has_value()) {
      menu_model_.InsertItemWithStringIdAt(
          copy_index.value() + 1, IDC_COPY_CLEAN_LINK, IDS_COPY_CLEAN_LINK);
    }
  }

  BuildAIChatMenu();

  // Add Open Link in Split View
  if (CanOpenSplitViewForWebContents(source_web_contents_->GetWeakPtr()) &&
      params_.link_url.is_valid()) {
    index = menu_model_.GetIndexOfCommandId(IDC_CONTENT_CONTEXT_OPENLINKNEWTAB);
    CHECK(index.has_value());

    menu_model_.InsertItemWithStringIdAt(
        index.value() + 1, IDC_CONTENT_CONTEXT_OPENLINK_SPLIT_VIEW,
        IDS_CONTENT_CONTEXT_SPLIT_VIEW);
  }
}

void BraveRenderViewContextMenu::NotifyMenuShown() {
  auto* cb = BraveGetMenuShownCallback();
  if (!cb->is_null()) {
    std::move(*cb).Run(this);
  }
}
