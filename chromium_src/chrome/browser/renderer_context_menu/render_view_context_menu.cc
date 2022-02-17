// Copyright 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_context_menu/render_view_context_menu.h"

#include "brave/browser/autocomplete/brave_autocomplete_scheme_classifier.h"
#include "brave/browser/ipfs/import/ipfs_import_controller.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/renderer_context_menu/brave_spelling_options_submenu_observer.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/components/translate/core/common/buildflags.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_provider_client.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/common/channel_info.h"
#include "components/omnibox/browser/autocomplete_classifier.h"
#include "components/omnibox/browser/autocomplete_controller.h"
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

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
#include "brave/browser/translate/brave_translate_utils.h"
#endif

// Our .h file creates a masquerade for RenderViewContextMenu.  Switch
// back to the Chromium one for the Chromium implementation.
#undef RenderViewContextMenu
#define RenderViewContextMenu RenderViewContextMenu_Chromium

namespace {

GURL GetSelectionNavigationURL(Profile* profile, const std::u16string& text) {
  AutocompleteMatch match;
  AutocompleteClassifier classifier(
      std::make_unique<AutocompleteController>(
          std::make_unique<ChromeAutocompleteProviderClient>(profile),
          AutocompleteClassifier::DefaultOmniboxProviders()),
      std::make_unique<BraveAutocompleteSchemeClassifier>(profile));
  classifier.Classify(text, false, false,
                      metrics::OmniboxEventProto::INVALID_SPEC, &match, NULL);
  classifier.Shutdown();
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
                         Profile* profile) {
  Browser* browser = chrome::FindLastActiveWithProfile(profile);
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
    default:
      RenderViewContextMenu_Chromium::ExecuteCommand(id, event_flags);
  }
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
  int index =
      menu_model_.GetIndexOfCommandId(IDC_CONTENT_CONTEXT_INSPECTELEMENT);
  if (index == -1)
    return;
  if (!params_.selection_text.empty() &&
      params_.media_type == ContextMenuDataMediaType::kNone) {
    menu_model_.InsertSeparatorAt(index,
                                  ui::MenuSeparatorType::NORMAL_SEPARATOR);

    menu_model_.InsertItemWithStringIdAt(
        index, IDC_CONTENT_CONTEXT_IMPORT_SELECTED_TEXT_IPFS,
        IDS_CONTENT_CONTEXT_IMPORT_IPFS_SELECTED_TEXT);
    SeIpfsIconAt(index);
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
  menu_model_.InsertSeparatorAt(index, ui::MenuSeparatorType::NORMAL_SEPARATOR);
  menu_model_.InsertSubMenuWithStringIdAt(
      index, IDC_CONTENT_CONTEXT_IMPORT_IPFS, IDS_CONTENT_CONTEXT_IMPORT_IPFS,
      &ipfs_submenu_model_);
  SeIpfsIconAt(index);
}
#endif

void BraveRenderViewContextMenu::InitMenu() {
  RenderViewContextMenu_Chromium::InitMenu();

  int index = -1;
#if BUILDFLAG(ENABLE_TOR)
  // Add Open Link with Tor
  if (!TorProfileServiceFactory::IsTorDisabled() &&
      !params_.link_url.is_empty()) {
    const Browser* browser = GetBrowser();
    const bool is_app = browser && browser->is_type_app();

    index = menu_model_.GetIndexOfCommandId(
        IDC_CONTENT_CONTEXT_OPENLINKOFFTHERECORD);
    DCHECK_NE(index, -1);

    menu_model_.InsertItemWithStringIdAt(
        index + 1,
        IDC_CONTENT_CONTEXT_OPENLINKTOR,
        is_app ? IDS_CONTENT_CONTEXT_OPENLINKTOR_INAPP
               : IDS_CONTENT_CONTEXT_OPENLINKTOR);
  }
#endif

#if BUILDFLAG(ENABLE_IPFS)
  BuildIPFSMenu();
#endif

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
  const bool remove_translate =
      !translate::IsInternalTranslationEnabled(GetProfile());
#else
  const bool remove_translate = true;
#endif  // BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)

  // Only show the translate item when go-translate is enabled.
  // This removes menu item, but keeps the duplicated separator. The duplicated
  // separator is removed in |BraveRenderViewContextMenuViews::Show|
  if (remove_translate) {
    index = menu_model_.GetIndexOfCommandId(IDC_CONTENT_CONTEXT_TRANSLATE);
    if (index != -1)
      menu_model_.RemoveItemAt(index);
  }
}
