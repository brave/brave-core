/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speedreader/speedreader_tab_helper.h"

#include <initializer_list>
#include <string>
#include <utility>

#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/speedreader/page_distiller.h"
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
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/dom_distiller/content/browser/distillable_page_utils.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_change_registrar.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/reload_type.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/resource/resource_bundle.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/brave_browser_window.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#endif

namespace speedreader {

std::u16string GetSpeedreaderData(
    std::initializer_list<std::pair<base::StringPiece, int>> resources) {
  std::u16string result = u"speedreaderData = {";
  for (const auto& r : resources) {
    auto text = brave_l10n::GetLocalizedResourceUTF16String(r.second);
    // Make sure that the text doesn't contain js injection
    base::ReplaceChars(text, u"\"", u"\\\"", &text);
    result += base::StrCat({base::UTF8ToUTF16(r.first), u": \"", text, u"\","});
  }

  return result + u"}\n\n";
}

constexpr const char* kPropertyPrefNames[] = {
    kSpeedreaderPrefTheme, kSpeedreaderPrefFontSize, kSpeedreaderPrefFontFamily,
    kSpeedreaderPrefContentStyle};

SpeedreaderTabHelper::SpeedreaderTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<SpeedreaderTabHelper>(*web_contents),
      PageDistiller(web_contents) {
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

  dom_distiller::AddObserver(web_contents, this);

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

// static
PageDistiller* SpeedreaderTabHelper::GetPageDistiller(
    content::WebContents* contents) {
  auto* tab_helper = SpeedreaderTabHelper::FromWebContents(contents);
  if (!tab_helper) {
    return nullptr;
  }
  return tab_helper;
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
  return SpeedreaderServiceFactory::GetForProfile(GetProfile())->IsEnabled();
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
  GetDistilledHTML(base::BindOnce(&SpeedreaderTabHelper::OnGetDocumentSource,
                                  weak_factory_.GetWeakPtr()));

  SetNextRequestState(DistillState::kReaderModePending);
  single_shot_next_request_ = true;
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
        // Determine readability on DOMContentLoaded.
        SetNextRequestState(DistillState::kNone);
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
  if (PageStateIsDistilled(distill_state_)) {
    UpdateState(State::kDistilled);
  } else if (PageSupportsDistillation(distill_state_)) {
    UpdateState(State::kDistillable);
  } else {
    UpdateState(State::kNotDistillable);
  }

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
  if (PageDistillState() == DistillState::kNone) {
    auto* rewriter_service =
        g_brave_browser_process->speedreader_rewriter_service();
    if (rewriter_service && rewriter_service->URLLooksReadable(
                                web_contents()->GetLastCommittedURL())) {
      distill_state_ = DistillState::kPageProbablyReadable;
      UpdateButtonIfNeeded();
    }
    return;
  }

  if (!PageWantsDistill(distill_state_)) {
    return;
  }

  static base::NoDestructor<std::u16string> kSpeedreaderData(GetSpeedreaderData(
      {{"showOriginalLinkText", IDS_SPEEDREADER_SHOW_ORIGINAL_PAGE_LINK},
       {"minutesText", IDS_SPEEDREADER_MINUTES_TEXT}}));

  static base::NoDestructor<std::u16string> kJsScript(base::UTF8ToUTF16(
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
          IDR_SPEEDREADER_JS_DESKTOP)));

  static base::NoDestructor<std::u16string> kLoadScript(*kSpeedreaderData +
                                                        *kJsScript);

  render_frame_host->ExecuteJavaScriptInIsolatedWorld(
      *kLoadScript, base::DoNothing(), ISOLATED_WORLD_ID_BRAVE_INTERNAL);
}

void SpeedreaderTabHelper::OnVisibilityChanged(content::Visibility visibility) {
  is_visible_ = visibility != content::Visibility::HIDDEN;
}

void SpeedreaderTabHelper::WebContentsDestroyed() {
  pref_change_registrar_.reset();
  dom_distiller::RemoveObserver(web_contents(), this);
  content_rules_ = nullptr;
  SetWebContents(nullptr);
  HideBubble();
}

bool SpeedreaderTabHelper::IsPageDistillationAllowed() {
  return speedreader::PageWantsDistill(distill_state_);
}

bool SpeedreaderTabHelper::IsPageContentPresent() {
  return !single_show_content_.empty();
}

std::string SpeedreaderTabHelper::TakePageContent() {
  return std::move(single_show_content_);
}

void SpeedreaderTabHelper::OnDistillComplete(DistillationResult result) {
  if (result != DistillationResult::kSuccess) {
    SetNextRequestState(DistillState::kNone);
    return;
  }

  // Perform a state transition
  if (distill_state_ == DistillState::kSpeedreaderModePending) {
    distill_state_ = DistillState::kSpeedreaderMode;
  } else if (distill_state_ == DistillState::kReaderModePending) {
    if (result == DistillationResult::kSuccess) {
      distill_state_ = DistillState::kReaderMode;
    } else {
      distill_state_ = DistillState::kPageProbablyReadable;
    }
  } else {
    // We got here via an already cached page.
    DCHECK(distill_state_ == DistillState::kSpeedreaderMode ||
           distill_state_ == DistillState::kReaderMode);
  }

  UpdateButtonIfNeeded();
}

void SpeedreaderTabHelper::OnResult(
    const dom_distiller::DistillabilityResult& result) {
  if (PageDistillState() == DistillState::kNone) {
    if (result.is_distillable) {
      // Page detected as non-readable by URL, but readable by content.
      distill_state_ = DistillState::kPageProbablyReadable;
      UpdateButtonIfNeeded();
    }
  }
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
      script, base::DoNothing(), ISOLATED_WORLD_ID_BRAVE_INTERNAL);
}

void SpeedreaderTabHelper::OnGetDocumentSource(bool success, std::string html) {
  DCHECK(single_shot_next_request_);
  if (!success) {
    // TODO(boocmp): Show error dialog [Distillation failed on this page].
    SetNextRequestState(DistillState::kPageProbablyReadable);
    return;
  }

  single_show_content_.swap(html);
  ReloadContents();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(SpeedreaderTabHelper);

}  // namespace speedreader
