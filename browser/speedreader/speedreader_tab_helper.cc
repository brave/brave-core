/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/speedreader/speedreader_tab_helper.h"

#include <utility>

#include "base/bind.h"
#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/ui/speedreader/speedreader_bubble_view.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/speedreader/common/constants.h"
#include "brave/components/speedreader/common/features.h"
#include "brave/components/speedreader/speedreader_extended_info_handler.h"
#include "brave/components/speedreader/speedreader_pref_names.h"
#include "brave/components/speedreader/speedreader_rewriter_service.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_change_registrar.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/reload_type.h"
#include "content/public/browser/web_contents.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/brave_browser_window.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#endif

namespace speedreader {

namespace test {

const std::u16string* g_show_original_link_title = nullptr;

void SetShowOriginalLinkTitle(const std::u16string* title) {
  g_show_original_link_title = title;
}

}  // namespace test

constexpr const char* kPropertyPrefNames[] = {
    kSpeedreaderPrefTheme, kSpeedreaderPrefFontSize, kSpeedreaderPrefFontFamily,
    kSpeedreaderPrefContentStyle};

SpeedreaderTabHelper::SpeedreaderTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<SpeedreaderTabHelper>(*web_contents) {
  pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
  pref_change_registrar_->Init(GetProfile()->GetPrefs());
  pref_change_registrar_->Add(
      kSpeedreaderPrefEnabled,
      base::BindRepeating(&SpeedreaderTabHelper::OnPrefChanged,
                          weak_factory_.GetWeakPtr()));

  for (const auto* pref_name : kPropertyPrefNames) {
    pref_change_registrar_->Add(
        pref_name,
        base::BindRepeating(&SpeedreaderTabHelper::OnPropertyPrefChanged,
                            weak_factory_.GetWeakPtr()));
  }

  content_rules_ = HostContentSettingsMapFactory::GetForProfile(
      web_contents->GetBrowserContext());
}

SpeedreaderTabHelper::~SpeedreaderTabHelper() {
  DCHECK(!pref_change_registrar_);
  DCHECK(!speedreader_bubble_);
  DCHECK(!content_rules_);
}

// static
void SpeedreaderTabHelper::MaybeCreateForWebContents(
    content::WebContents* contents) {
  if (base::FeatureList::IsEnabled(speedreader::kSpeedreaderFeature)) {
    SpeedreaderTabHelper::CreateForWebContents(contents);
  }
}

// static
void SpeedreaderTabHelper::BindSpeedreaderHost(
    mojo::PendingAssociatedReceiver<mojom::SpeedreaderHost> receiver,
    content::RenderFrameHost* rfh) {
  auto* sender = content::WebContents::FromRenderFrameHost(rfh);
  if (!sender)
    return;
  auto* tab_helper = SpeedreaderTabHelper::FromWebContents(sender);
  if (!tab_helper)
    return;
  tab_helper->BindReceiver(std::move(receiver));
}

void SpeedreaderTabHelper::BindReceiver(
    mojo::PendingAssociatedReceiver<mojom::SpeedreaderHost> receiver) {
  receiver_.reset();
  receiver_.Bind(std::move(receiver));
}

base::WeakPtr<SpeedreaderTabHelper> SpeedreaderTabHelper::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

bool SpeedreaderTabHelper::IsSpeedreaderEnabled() const {
#if BUILDFLAG(IS_ANDROID)
  return true;  // skip the preference storage for now
#else
  return SpeedreaderServiceFactory::GetForProfile(GetProfile())->IsEnabled();
#endif
}

bool SpeedreaderTabHelper::IsEnabledForSite() {
  return IsEnabledForSite(web_contents()->GetLastCommittedURL());
}

bool SpeedreaderTabHelper::IsEnabledForSite(const GURL& url) {
  if (!IsSpeedreaderEnabled())
    return false;
  return speedreader::IsEnabledForSite(content_rules_, url);
}

void SpeedreaderTabHelper::ProcessIconClick() {
  switch (distill_state_) {
    case DistillState::kSpeedreaderMode:
      ShowSpeedreaderBubble();
      break;
    case DistillState::kSpeedreaderOnDisabledPage:
      if (original_page_shown_ &&
          IsEnabledForSite(web_contents()->GetLastCommittedURL())) {
        ReloadContents();
      } else {
        ShowSpeedreaderBubble();
      }
      break;
    case DistillState::kReaderMode:
      SetNextRequestState(DistillState::kPageProbablyReadable);
      ClearPersistedData();
      ReloadContents();
      break;
    case DistillState::kPageProbablyReadable:
      SingleShotSpeedreader();
      break;
    default:
      NOTREACHED();
  }
}

void SpeedreaderTabHelper::MaybeToggleEnabledForSite(bool on) {
  if (!IsSpeedreaderEnabled())
    return;

  const bool enabled = speedreader::IsEnabledForSite(
      content_rules_, web_contents()->GetLastCommittedURL());
  if (enabled == on)
    return;

  speedreader::SetEnabledForSite(content_rules_,
                                 web_contents()->GetLastCommittedURL(), on);
  ClearPersistedData();
  ReloadContents();
}

void SpeedreaderTabHelper::SingleShotSpeedreader() {
  single_shot_next_request_ = true;

  // Refresh the page so it runs through the speedreader throttle
  ReloadContents();

  // Determine if bubble should be shown automatically
  auto* speedreader_service =
      SpeedreaderServiceFactory::GetForProfile(GetProfile());
  if (speedreader_service->ShouldPromptUserToEnable()) {
    ShowReaderModeBubble();
    speedreader_service->IncrementPromptCount();
  }
}

SpeedreaderBubbleView* SpeedreaderTabHelper::speedreader_bubble_view() const {
  return speedreader_bubble_;
}

bool SpeedreaderTabHelper::MaybeUpdateCachedState(
    content::NavigationHandle* handle) {
  auto* entry = handle->GetNavigationEntry();
  if (!entry || handle->GetRestoreType() != content::RestoreType::kRestored) {
    return false;
  }
  auto* speedreader_service =
      SpeedreaderServiceFactory::GetForProfile(GetProfile());

  const DistillState state =
      SpeedreaderExtendedInfoHandler::GetCachedMode(entry, speedreader_service);
  const bool cached = state != DistillState::kUnknown;
  if (cached) {
    distill_state_ = state;
  } else {
    SpeedreaderExtendedInfoHandler::ClearPersistedData(entry);
  }
  return cached;
}

void SpeedreaderTabHelper::UpdateActiveState(const GURL& url) {
  if (single_shot_next_request_) {
    SetNextRequestState(DistillState::kReaderModePending);
    return;
  }

  if (show_original_page_) {
    SetNextRequestState(DistillState::kSpeedreaderOnDisabledPage);
    return;
  }

  // Work only with casual main frame navigations.
  if (url.SchemeIsHTTPOrHTTPS()) {
    auto* rewriter_service =
        g_brave_browser_process->speedreader_rewriter_service();
    if (rewriter_service->URLLooksReadable(url)) {
      VLOG(2) << __func__ << "URL passed speedreader heuristic: " << url;
      if (!IsSpeedreaderEnabled()) {
        SetNextRequestState(DistillState::kPageProbablyReadable);
      } else if (!IsEnabledForSite(url)) {
        SetNextRequestState(DistillState::kSpeedreaderOnDisabledPage);
      } else {
        SetNextRequestState(DistillState::kSpeedreaderModePending);
      }
      return;
    }
  }
  SetNextRequestState(DistillState::kNone);
}

void SpeedreaderTabHelper::SetNextRequestState(DistillState state) {
  distill_state_ = state;
  single_shot_next_request_ = false;
  show_original_page_ = false;
}

void SpeedreaderTabHelper::OnBubbleClosed() {
  speedreader_bubble_ = nullptr;
  UpdateButtonIfNeeded();

  // auto* contents = web_contents();
  // Browser* browser = chrome::FindBrowserWithWebContents(contents);
  // if (browser) {
  //   static_cast<BraveBrowserWindow*>(browser->window())->ShowSpeedreaderWebUIBubble(browser);
  // }
}

void SpeedreaderTabHelper::ShowSpeedreaderBubble() {
  ShowBubble(true);
}

void SpeedreaderTabHelper::ShowReaderModeBubble() {
  ShowBubble(false);
}

Profile* SpeedreaderTabHelper::GetProfile() const {
  auto* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  DCHECK(profile);
  return profile;
}

void SpeedreaderTabHelper::ShowBubble(bool is_bubble_speedreader) {
#if !BUILDFLAG(IS_ANDROID)
  auto* contents = web_contents();
  Browser* browser = chrome::FindBrowserWithWebContents(contents);
  DCHECK(browser);

  if (speedreader::IsSpeedreaderPanelV2Enabled()) {
    static_cast<BraveBrowserWindow*>(browser->window())
        ->ShowSpeedreaderWebUIBubble(browser);
    return;
  }

  speedreader_bubble_ =
      static_cast<BraveBrowserWindow*>(browser->window())
          ->ShowSpeedreaderBubble(this, is_bubble_speedreader);
#endif
}

void SpeedreaderTabHelper::HideBubble() {
  if (speedreader_bubble_) {
    speedreader_bubble_->Hide();
    speedreader_bubble_ = nullptr;
  }
}

void SpeedreaderTabHelper::OnShowOriginalPage() {
  show_original_page_ = distill_state_ == DistillState::kSpeedreaderMode;
  ReloadContents();
}

void SpeedreaderTabHelper::SetTheme(Theme theme) {
  auto* speedreader_service =
      SpeedreaderServiceFactory::GetForProfile(GetProfile());
  if (!speedreader_service)
    return;

  if (speedreader_service->GetTheme() == theme)
    return;
  speedreader_service->SetTheme(theme);
}

Theme SpeedreaderTabHelper::GetTheme() {
  const Theme theme =
      SpeedreaderServiceFactory::GetForProfile(GetProfile())->GetTheme();
  if (theme == Theme::kNone) {
    switch (dark_mode::GetActiveBraveDarkModeType()) {
      case dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK:
        return Theme::kDark;
      case dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT:
      case dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT:
        return Theme::kLight;
    }
  }
  return theme;
}

void SpeedreaderTabHelper::SetFontFamily(FontFamily font) {
  auto* speedreader_service =
      SpeedreaderServiceFactory::GetForProfile(GetProfile());
  if (!speedreader_service)
    return;
  if (speedreader_service->GetFontFamily() == font)
    return;

  speedreader_service->SetFontFamily(font);
}

FontFamily SpeedreaderTabHelper::GetFontFamily() {
  return SpeedreaderServiceFactory::GetForProfile(GetProfile())
      ->GetFontFamily();
}

void SpeedreaderTabHelper::SetFontSize(FontSize size) {
  auto* speedreader_service =
      SpeedreaderServiceFactory::GetForProfile(GetProfile());
  if (!speedreader_service)
    return;
  if (speedreader_service->GetFontSize() == size)
    return;

  speedreader_service->SetFontSize(size);
}

FontSize SpeedreaderTabHelper::GetFontSize() const {
  return SpeedreaderServiceFactory::GetForProfile(GetProfile())->GetFontSize();
}

void SpeedreaderTabHelper::SetContentStyle(ContentStyle style) {
  auto* speedreader_service =
      SpeedreaderServiceFactory::GetForProfile(GetProfile());
  if (!speedreader_service)
    return;
  if (speedreader_service->GetContentStyle() == style)
    return;

  speedreader_service->SetContentStyle(style);
}

ContentStyle SpeedreaderTabHelper::GetContentStyle() {
  return SpeedreaderServiceFactory::GetForProfile(GetProfile())
      ->GetContentStyle();
}

std::string SpeedreaderTabHelper::GetCurrentSiteURL() {
  return web_contents()->GetLastCommittedURL().host();
}

void SpeedreaderTabHelper::ClearPersistedData() {
  if (auto* entry = web_contents()->GetController().GetLastCommittedEntry()) {
    SpeedreaderExtendedInfoHandler::ClearPersistedData(entry);
  }
}

void SpeedreaderTabHelper::ReloadContents() {
  web_contents()->GetController().Reload(content::ReloadType::NORMAL, false);
}

void SpeedreaderTabHelper::ProcessNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument() ||
      MaybeUpdateCachedState(navigation_handle)) {
    return;
  }

  original_page_shown_ = show_original_page_;

  UpdateActiveState(navigation_handle->GetURL());
  UpdateButtonIfNeeded();
}

void SpeedreaderTabHelper::OnPrefChanged() {
  const bool is_speedreader_enabled = IsSpeedreaderEnabled();

  switch (distill_state_) {
    case DistillState::kUnknown:
    case DistillState::kNone:
      break;  // Nothing to do.
    case DistillState::kPageProbablyReadable:
      if (is_speedreader_enabled) {
        distill_state_ = DistillState::kSpeedreaderOnDisabledPage;
      }
      break;
    case DistillState::kSpeedreaderMode:
      if (!is_speedreader_enabled) {
        distill_state_ = DistillState::kReaderMode;
      }
      break;
    case DistillState::kReaderMode:
      if (is_speedreader_enabled) {
        distill_state_ = DistillState::kSpeedreaderMode;
      } else {
        distill_state_ = DistillState::kSpeedreaderOnDisabledPage;
      }
      break;
    case DistillState::kSpeedreaderOnDisabledPage: {
      if (!is_speedreader_enabled) {
        distill_state_ = DistillState::kPageProbablyReadable;
      }
      break;
    }
    default:
      break;
  }

  UpdateButtonIfNeeded();
}

void SpeedreaderTabHelper::OnPropertyPrefChanged(const std::string& path) {
  DCHECK(base::Contains(kPropertyPrefNames, path));

  auto* speedreader_service =
      SpeedreaderServiceFactory::GetForProfile(GetProfile());
  if (!speedreader_service)
    return;
  if (!PageStateIsDistilled(distill_state_))
    return;

  if (path == kSpeedreaderPrefTheme) {
    SetDocumentAttribute("data-theme", speedreader_service->GetThemeName());
  } else if (path == kSpeedreaderPrefFontFamily) {
    SetDocumentAttribute("data-font-family",
                         speedreader_service->GetFontFamilyName());
  } else if (path == kSpeedreaderPrefFontSize) {
    SetDocumentAttribute("data-font-size",
                         speedreader_service->GetFontSizeName());
  } else if (path == kSpeedreaderPrefContentStyle) {
    SetDocumentAttribute("data-content-style",
                         speedreader_service->GetContentStyleName());
  }
}

void SpeedreaderTabHelper::UpdateButtonIfNeeded() {
  if (!is_visible_)
    return;
#if !BUILDFLAG(IS_ANDROID)
  if (const auto* browser =
          chrome::FindBrowserWithWebContents(web_contents())) {
    browser->window()->UpdatePageActionIcon(PageActionIconType::kReaderMode);
  }
#endif
}

void SpeedreaderTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  ProcessNavigation(navigation_handle);
}

void SpeedreaderTabHelper::DidRedirectNavigation(
    content::NavigationHandle* navigation_handle) {
  ProcessNavigation(navigation_handle);
}

void SpeedreaderTabHelper::DidStopLoading() {
  auto* entry = web_contents()->GetController().GetLastCommittedEntry();
  if (entry) {
    SpeedreaderExtendedInfoHandler::PersistMode(entry, distill_state_);
  }
}

void SpeedreaderTabHelper::DOMContentLoaded(
    content::RenderFrameHost* render_frame_host) {
  if (!PageWantsDistill(distill_state_))
    return;

  constexpr const char16_t kAddShowOriginalPageLink[] =
      uR"js(
    (function() {
      // element id is hardcoded in extractor.rs
      const link = document.
        getElementById('c93e2206-2f31-4ddc-9828-2bb8e8ed940e');
      if (!link)
        return;
      link.innerText = "$1";
      link.addEventListener('click', (e) => {
        window.speedreader.showOriginalPage();
      })
    })();
  )js";

  auto link_text = brave_l10n::GetLocalizedResourceUTF16String(
      IDS_SPEEDREADER_SHOW_ORIGINAL_PAGE_LINK);
  if (test::g_show_original_link_title) {
    link_text = *test::g_show_original_link_title;
  }

  // Make sure that the link text doesn't contain js injection
  base::ReplaceChars(link_text, u"\"", u"\\\"", &link_text);

  const auto script = base::ReplaceStringPlaceholders(kAddShowOriginalPageLink,
                                                      link_text, nullptr);

  render_frame_host->ExecuteJavaScriptInIsolatedWorld(script, base::DoNothing(),
                                                      kIsolatedWorldId);
}

void SpeedreaderTabHelper::OnVisibilityChanged(content::Visibility visibility) {
  is_visible_ = visibility != content::Visibility::HIDDEN;
}

void SpeedreaderTabHelper::WebContentsDestroyed() {
  pref_change_registrar_.reset();
  content_rules_ = nullptr;
  HideBubble();
}

bool SpeedreaderTabHelper::IsPageDistillationAllowed() {
  return speedreader::PageWantsDistill(distill_state_);
}

void SpeedreaderTabHelper::OnDistillComplete() {
  // Perform a state transition
  if (distill_state_ == DistillState::kSpeedreaderModePending) {
    distill_state_ = DistillState::kSpeedreaderMode;
  } else if (distill_state_ == DistillState::kReaderModePending) {
    distill_state_ = DistillState::kReaderMode;
  } else {
    // We got here via an already cached page.
    DCHECK(distill_state_ == DistillState::kSpeedreaderMode ||
           distill_state_ == DistillState::kReaderMode);
  }

  UpdateButtonIfNeeded();
}

void SpeedreaderTabHelper::SetDocumentAttribute(const std::string& attribute,
                                                const std::string& value) {
  constexpr const char16_t kSetAttribute[] =
      uR"js(
    (function() {
      const attribute = '$1'
      const value = '$2'
      if (value == '') {
        document.documentElement.removeAttribute(attribute)
      } else {
        document.documentElement.setAttribute(attribute, value)
      }
    })();
  )js";

  const auto script = base::ReplaceStringPlaceholders(
      kSetAttribute, {base::UTF8ToUTF16(attribute), base::UTF8ToUTF16(value)},
      nullptr);

  web_contents()->GetPrimaryMainFrame()->ExecuteJavaScriptInIsolatedWorld(
      script, base::DoNothing(), kIsolatedWorldId);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(SpeedreaderTabHelper);

}  // namespace speedreader
