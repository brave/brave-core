/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/renderer_context_menu/render_view_context_menu.h"

#include "brave/browser/autocomplete/brave_autocomplete_scheme_classifier.h"
#include "brave/browser/ipfs/import/ipfs_import_controller.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/renderer_context_menu/brave_spelling_options_submenu_observer.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/browser_dialogs.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_provider_client.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/common/channel_info.h"
#include "components/omnibox/browser/autocomplete_classifier.h"
#include "components/omnibox/browser/autocomplete_controller.h"
#include "components/omnibox/browser/autocomplete_match_type.h"
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

#if BUILDFLAG(ENABLE_IPFS)
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/browser/ipfs/ipfs_tab_helper.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/ipfs_utils.h"
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

absl::optional<GURL> GetSelectedURL(Profile* profile,
                                    const std::u16string& text) {
  auto match = GetAutocompleteMatchForText(profile, text);
  if (match.type != AutocompleteMatchType::URL_WHAT_YOU_TYPED) {
    return absl::nullopt;
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

#define BRAVE_APPEND_SEARCH_PROVIDER \
  if (GetProfile()->IsOffTheRecord()) { \
    selection_navigation_url_ = \
        GetSelectionNavigationURL(GetProfile(), params_.selection_text); \
    if (!selection_navigation_url_.is_valid()) \
      return; \
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
        browser->profile()->GetOriginalProfile() == profile)
      return true;
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
  if (!web_contents)
    return;

  brave::ShowTextRecognitionDialog(web_contents.get(), image);
}
#endif

}  // namespace

BraveRenderViewContextMenu::BraveRenderViewContextMenu(
    content::RenderFrameHost& render_frame_host,
    const content::ContextMenuParams& params)
    : RenderViewContextMenu_Chromium(render_frame_host, params)
#if BUILDFLAG(ENABLE_IPFS)
      ,
      ipfs_submenu_model_(this)
#endif
{
}

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
#if BUILDFLAG(ENABLE_IPFS)
    case IDC_CONTENT_CONTEXT_IMPORT_IPFS:
    case IDC_CONTENT_CONTEXT_IMPORT_IPFS_PAGE:
    case IDC_CONTENT_CONTEXT_IMPORT_IMAGE_IPFS:
    case IDC_CONTENT_CONTEXT_IMPORT_VIDEO_IPFS:
    case IDC_CONTENT_CONTEXT_IMPORT_AUDIO_IPFS:
    case IDC_CONTENT_CONTEXT_IMPORT_LINK_IPFS:
    case IDC_CONTENT_CONTEXT_IMPORT_SELECTED_TEXT_IPFS:
      return IsIPFSCommandIdEnabled(id);
#endif
    case IDC_CONTENT_CONTEXT_OPENLINKTOR:
#if BUILDFLAG(ENABLE_TOR)
      if (brave::IsTorDisabledForProfile(GetProfile()))
        return false;

      return params_.link_url.is_valid() &&
             IsURLAllowedInIncognito(params_.link_url, browser_context_) &&
             !GetProfile()->IsTor();
#else
      return false;
#endif
    default:
      return RenderViewContextMenu_Chromium::IsCommandIdEnabled(id);
  }
}
#if BUILDFLAG(ENABLE_IPFS)
void BraveRenderViewContextMenu::ExecuteIPFSCommand(int id, int event_flags) {
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(source_web_contents_);
  if (!helper)
    return;
  auto* controller = helper->GetImportController();
  if (!controller)
    return;
  switch (id) {
    case IDC_CONTENT_CONTEXT_IMPORT_IPFS_PAGE:
      helper->ImportCurrentPageToIpfs();
      break;
    case IDC_CONTENT_CONTEXT_IMPORT_IMAGE_IPFS:
    case IDC_CONTENT_CONTEXT_IMPORT_VIDEO_IPFS:
    case IDC_CONTENT_CONTEXT_IMPORT_AUDIO_IPFS: {
      if (params_.src_url.SchemeIsFile()) {
        base::FilePath path;
        if (net::FileURLToFilePath(params_.src_url, &path) && !path.empty()) {
          controller->ImportFileToIpfs(path, std::string());
        }
      } else {
        controller->ImportLinkToIpfs(params_.src_url);
      }
    }; break;
    case IDC_CONTENT_CONTEXT_IMPORT_LINK_IPFS:
      controller->ImportLinkToIpfs(params_.link_url);
      break;
    case IDC_CONTENT_CONTEXT_IMPORT_SELECTED_TEXT_IPFS:
      controller->ImportTextToIpfs(base::UTF16ToUTF8(params_.selection_text));
      break;
  }
}
#endif

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
#if BUILDFLAG(ENABLE_IPFS)
    case IDC_CONTENT_CONTEXT_IMPORT_IPFS_PAGE:
    case IDC_CONTENT_CONTEXT_IMPORT_IMAGE_IPFS:
    case IDC_CONTENT_CONTEXT_IMPORT_VIDEO_IPFS:
    case IDC_CONTENT_CONTEXT_IMPORT_AUDIO_IPFS:
    case IDC_CONTENT_CONTEXT_IMPORT_LINK_IPFS:
    case IDC_CONTENT_CONTEXT_IMPORT_SELECTED_TEXT_IPFS:
      ExecuteIPFSCommand(id, event_flags);
      break;
#endif
#if BUILDFLAG(ENABLE_TOR)
    case IDC_CONTENT_CONTEXT_OPENLINKTOR:
      TorProfileManager::SwitchToTorProfile(
          GetProfile(),
          base::BindRepeating(OnTorProfileCreated, params_.link_url,
                              HasAlreadyOpenedTorWindow(GetProfile())));
      break;
#endif
#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
    case IDC_CONTENT_CONTEXT_COPY_TEXT_FROM_IMAGE:
      CopyTextFromImage();
      break;
#endif
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

#if BUILDFLAG(ENABLE_IPFS)
bool BraveRenderViewContextMenu::IsIPFSCommandIdEnabled(int command) const {
  if (!ipfs::IsIpfsMenuEnabled(GetProfile()->GetPrefs()))
    return false;
  switch (command) {
    case IDC_CONTENT_CONTEXT_IMPORT_IPFS:
      return true;
    case IDC_CONTENT_CONTEXT_IMPORT_IPFS_PAGE:
      return source_web_contents_->GetURL().SchemeIsHTTPOrHTTPS() &&
             source_web_contents_->IsSavable();
    case IDC_CONTENT_CONTEXT_IMPORT_IMAGE_IPFS:
      return params_.has_image_contents;
    case IDC_CONTENT_CONTEXT_IMPORT_VIDEO_IPFS:
      return content_type_->SupportsGroup(
          ContextMenuContentType::ITEM_GROUP_MEDIA_VIDEO);
    case IDC_CONTENT_CONTEXT_IMPORT_AUDIO_IPFS:
      return content_type_->SupportsGroup(
          ContextMenuContentType::ITEM_GROUP_MEDIA_AUDIO);
    case IDC_CONTENT_CONTEXT_IMPORT_LINK_IPFS:
      return !params_.link_url.is_empty();
    case IDC_CONTENT_CONTEXT_IMPORT_SELECTED_TEXT_IPFS:
      return !params_.selection_text.empty() &&
             params_.media_type == ContextMenuDataMediaType::kNone;
    default:
      NOTREACHED();
  }
  return false;
}

void BraveRenderViewContextMenu::SeIpfsIconAt(int index) {
  auto& bundle = ui::ResourceBundle::GetSharedInstance();
  const auto& ipfs_logo = *bundle.GetImageSkiaNamed(IDR_BRAVE_IPFS_LOGO);
  ui::ImageModel model = ui::ImageModel::FromImageSkia(ipfs_logo);
  menu_model_.SetIcon(index, model);
}

void BraveRenderViewContextMenu::BuildIPFSMenu() {
  if (!ipfs::IsIpfsMenuEnabled(GetProfile()->GetPrefs()))
    return;
  absl::optional<size_t> index =
      menu_model_.GetIndexOfCommandId(IDC_CONTENT_CONTEXT_INSPECTELEMENT);
  if (!index.has_value())
    return;
  if (!params_.selection_text.empty() &&
      params_.media_type == ContextMenuDataMediaType::kNone) {
    menu_model_.InsertSeparatorAt(index.value(),
                                  ui::MenuSeparatorType::NORMAL_SEPARATOR);

    menu_model_.InsertItemWithStringIdAt(
        index.value(), IDC_CONTENT_CONTEXT_IMPORT_SELECTED_TEXT_IPFS,
        IDS_CONTENT_CONTEXT_IMPORT_IPFS_SELECTED_TEXT);
    SeIpfsIconAt(index.value());
    return;
  }

  auto page_url = source_web_contents_->GetURL();
  url::Origin page_origin = url::Origin::Create(page_url);
  if (page_url.SchemeIsHTTPOrHTTPS() &&
      !ipfs::IsAPIGateway(page_origin.GetURL(), chrome::GetChannel())) {
    ipfs_submenu_model_.AddItemWithStringId(
        IDC_CONTENT_CONTEXT_IMPORT_IPFS_PAGE,
        IDS_CONTENT_CONTEXT_IMPORT_IPFS_PAGE);
  }
  if (params_.has_image_contents) {
    ipfs_submenu_model_.AddItemWithStringId(
        IDC_CONTENT_CONTEXT_IMPORT_IMAGE_IPFS,
        IDS_CONTENT_CONTEXT_IMPORT_IPFS_IMAGE);
  }
  if (content_type_->SupportsGroup(
          ContextMenuContentType::ITEM_GROUP_MEDIA_VIDEO)) {
    ipfs_submenu_model_.AddItemWithStringId(
        IDC_CONTENT_CONTEXT_IMPORT_VIDEO_IPFS,
        IDS_CONTENT_CONTEXT_IMPORT_IPFS_VIDEO);
  }
  if (content_type_->SupportsGroup(
          ContextMenuContentType::ITEM_GROUP_MEDIA_AUDIO)) {
    ipfs_submenu_model_.AddItemWithStringId(
        IDC_CONTENT_CONTEXT_IMPORT_AUDIO_IPFS,
        IDS_CONTENT_CONTEXT_IMPORT_IPFS_AUDIO);
  }
  if (!params_.link_url.is_empty()) {
    ipfs_submenu_model_.AddItemWithStringId(
        IDC_CONTENT_CONTEXT_IMPORT_LINK_IPFS,
        IDS_CONTENT_CONTEXT_IMPORT_IPFS_LINK);
  }
  if (!ipfs_submenu_model_.GetItemCount())
    return;
  menu_model_.InsertSeparatorAt(index.value(),
                                ui::MenuSeparatorType::NORMAL_SEPARATOR);
  menu_model_.InsertSubMenuWithStringIdAt(
      index.value(), IDC_CONTENT_CONTEXT_IMPORT_IPFS,
      IDS_CONTENT_CONTEXT_IMPORT_IPFS, &ipfs_submenu_model_);
  SeIpfsIconAt(index.value());
}
#endif

void BraveRenderViewContextMenu::InitMenu() {
  RenderViewContextMenu_Chromium::InitMenu();

  absl::optional<size_t> index = menu_model_.GetIndexOfCommandId(
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
  if (!TorProfileServiceFactory::IsTorDisabled() &&
      content_type_->SupportsGroup(ContextMenuContentType::ITEM_GROUP_LINK) &&
      !params_.link_url.is_empty()) {
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
  if (!params_.link_url.is_empty() && params_.link_url.SchemeIsHTTPOrHTTPS()) {
    absl::optional<size_t> link_index =
        menu_model_.GetIndexOfCommandId(IDC_CONTENT_CONTEXT_COPYLINKLOCATION);
    if (link_index.has_value()) {
      menu_model_.InsertItemWithStringIdAt(
          link_index.value() + 1, IDC_COPY_CLEAN_LINK, IDS_COPY_CLEAN_LINK);
    }
  }
  if (GetSelectedURL(GetProfile(), params_.selection_text).has_value()) {
    absl::optional<size_t> copy_index =
        menu_model_.GetIndexOfCommandId(IDC_CONTENT_CONTEXT_COPY);
    if (copy_index.has_value() &&
        !menu_model_.GetIndexOfCommandId(IDC_COPY_CLEAN_LINK).has_value()) {
      menu_model_.InsertItemWithStringIdAt(
          copy_index.value() + 1, IDC_COPY_CLEAN_LINK, IDS_COPY_CLEAN_LINK);
    }
  }
#if BUILDFLAG(ENABLE_IPFS)
  BuildIPFSMenu();
#endif
}
