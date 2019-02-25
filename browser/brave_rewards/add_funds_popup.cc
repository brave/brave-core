/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/add_funds_popup.h"

#include <memory>
#include <string>
#include <utility>

#include "base/base64.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/guest_view/browser/guest_view_base.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/common/referrer.h"
#include "net/base/escape.h"
#include "ui/base/page_transition_types.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/widget/widget.h"
#include "url/url_constants.h"

using content::WebContents;

namespace {

constexpr int kPopupPreferredHeight = 800;
constexpr int kPopupPreferredWidth = 1100;

// URL to open in the popup.
const char kAddFundsPopupUrl[] = "https://uphold-widget.brave.com/index.php";

// Content permission URLs.
const GURL kAddFundsUrl("https://uphold-widget.brave.com");
const GURL kUpholdUrl("https://uphold.com");
const GURL kNetverifyUrl("https://netverify.com");
const GURL kTypekitUrl("https://use.typekit.net");
const GURL kFirstPartyUrl("https://firstParty");

const std::map<std::string, std::string> kCurrencyToNetworkMap {
  {"BTC", "bitcoin"},
  {"BAT", "ethereum"},
  {"ETH", "ethereum"},
  {"LTC", "litecoin"}
};

class PopupContentSettingsBase {
 public:
  explicit PopupContentSettingsBase(Profile* profile);
  virtual ~PopupContentSettingsBase();

 private:
  // Allow settings.
  void Allow();
  void AllowShieldsFingerprinting(HostContentSettingsMap* map);
  void AllowShieldsCookies(HostContentSettingsMap* map);
  void AllowShieldsScripts(HostContentSettingsMap* map);
  void AllowCameraAccess(HostContentSettingsMap* map);
  void AllowAutoplay(HostContentSettingsMap* map);

  // Reset settings to the original values.
  void Reset();
  void ResetShieldsFingerprinting(HostContentSettingsMap* map);
  void ResetShieldsCookies(HostContentSettingsMap* map);
  void ResetShieldsScripts(HostContentSettingsMap* map);
  void ResetCameraAccess(HostContentSettingsMap* map);
  void ResetAutoplay(HostContentSettingsMap* map);

  // Generic settings routines.
  ContentSetting SetContentSetting(HostContentSettingsMap* map,
                                   const GURL& host,
                                   const GURL& secondary,
                                   ContentSettingsType type,
                                   ContentSetting setting,
                                   const std::string& resource_identifier,
                                   bool include_host_subdomains = false);

  void ResetContentSetting(HostContentSettingsMap* map,
                           const GURL& host,
                           ContentSettingsType type,
                           ContentSetting setting);

  ContentSettingsPattern GURLToPattern(const GURL& gurl,
                                       bool wildcard_subdomains = false);

  // Profile for which content setting have been altered.
  Profile* profile_;

  // Original content settings.
  ContentSetting referrers_setting_;
  ContentSetting cookies_setting_;
  ContentSetting cookies_setting_fp_;
  ContentSetting fingerprinting_setting_;
  ContentSetting fingerprinting_setting_fp_;

  ContentSetting js_brave_;
  ContentSetting js_uphold_;
  ContentSetting js_netverify_;
  ContentSetting js_typekit_;

  ContentSetting camera_setting_;
  ContentSetting autoplay_setting_;

  DISALLOW_COPY_AND_ASSIGN(PopupContentSettingsBase);
};

PopupContentSettingsBase::PopupContentSettingsBase(Profile* profile)
    : profile_(profile) {
  Allow();
}

PopupContentSettingsBase::~PopupContentSettingsBase() {
  Reset();
}

void PopupContentSettingsBase::Allow() {
  DCHECK(profile_);
  HostContentSettingsMap* map =
      HostContentSettingsMapFactory::GetForProfile(profile_);
  DCHECK(map);

  AllowShieldsFingerprinting(map);
  AllowShieldsCookies(map);
  AllowShieldsScripts(map);
  AllowCameraAccess(map);
  AllowAutoplay(map);
}

void PopupContentSettingsBase::AllowShieldsFingerprinting(
    HostContentSettingsMap* map) {
  // Narrower scope first.
  fingerprinting_setting_fp_ = SetContentSetting(
      map, kAddFundsUrl, kFirstPartyUrl, CONTENT_SETTINGS_TYPE_PLUGINS,
      CONTENT_SETTING_ALLOW, brave_shields::kFingerprinting);
  // Wider scope.
  fingerprinting_setting_ = SetContentSetting(
      map, kAddFundsUrl, GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
      CONTENT_SETTING_ALLOW, brave_shields::kFingerprinting);
}

void PopupContentSettingsBase::AllowShieldsCookies(
    HostContentSettingsMap* map) {
  referrers_setting_ = SetContentSetting(
      map, kAddFundsUrl, GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
      CONTENT_SETTING_ALLOW, brave_shields::kReferrers);
  cookies_setting_fp_ = SetContentSetting(
      map, kAddFundsUrl, kFirstPartyUrl, CONTENT_SETTINGS_TYPE_PLUGINS,
      CONTENT_SETTING_ALLOW, brave_shields::kCookies);
  cookies_setting_ = SetContentSetting(
      map, kAddFundsUrl, GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
      CONTENT_SETTING_ALLOW, brave_shields::kCookies);
}

void PopupContentSettingsBase::AllowShieldsScripts(
    HostContentSettingsMap* map) {
  // Allow scripts from our host, uphold.com, netverify.com, and
  // use.typekit.net.
  js_brave_ = SetContentSetting(map, kAddFundsUrl, GURL(),
    CONTENT_SETTINGS_TYPE_JAVASCRIPT,
    CONTENT_SETTING_ALLOW, std::string());
  js_uphold_ = SetContentSetting(map, kUpholdUrl, GURL(),
    CONTENT_SETTINGS_TYPE_JAVASCRIPT,
    CONTENT_SETTING_ALLOW, std::string(), true);
  js_netverify_ = SetContentSetting(map, kNetverifyUrl, GURL(),
    CONTENT_SETTINGS_TYPE_JAVASCRIPT,
    CONTENT_SETTING_ALLOW, std::string(), true);
  js_typekit_ = SetContentSetting(map, kTypekitUrl, GURL(),
    CONTENT_SETTINGS_TYPE_JAVASCRIPT,
    CONTENT_SETTING_ALLOW, std::string());
}

void PopupContentSettingsBase::AllowCameraAccess(HostContentSettingsMap* map) {
  camera_setting_ = SetContentSetting(map, kAddFundsUrl, GURL(),
                                      CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA,
                                      CONTENT_SETTING_ALLOW, std::string());
}

void PopupContentSettingsBase::AllowAutoplay(HostContentSettingsMap* map) {
  autoplay_setting_ = SetContentSetting(map, kAddFundsUrl, GURL(),
                                        CONTENT_SETTINGS_TYPE_AUTOPLAY,
                                        CONTENT_SETTING_ALLOW, std::string());
}

void PopupContentSettingsBase::Reset() {
  DCHECK(profile_);
  HostContentSettingsMap* map =
      HostContentSettingsMapFactory::GetForProfile(profile_);
  DCHECK(map);

  ResetShieldsFingerprinting(map);
  ResetShieldsCookies(map);
  ResetShieldsScripts(map);
  ResetCameraAccess(map);
  ResetAutoplay(map);
}

void PopupContentSettingsBase::ResetShieldsFingerprinting(
    HostContentSettingsMap* map) {
  // Wide scope first.
  SetContentSetting(map, kAddFundsUrl, GURL(),
                    CONTENT_SETTINGS_TYPE_PLUGINS, fingerprinting_setting_,
                    brave_shields::kFingerprinting);
  // Then narrow scope.
  SetContentSetting(map, kAddFundsUrl, kFirstPartyUrl,
                    CONTENT_SETTINGS_TYPE_PLUGINS, fingerprinting_setting_fp_,
                    brave_shields::kFingerprinting);
}

void PopupContentSettingsBase::ResetShieldsCookies(
    HostContentSettingsMap* map) {
  SetContentSetting(map, kAddFundsUrl, GURL(),
                    CONTENT_SETTINGS_TYPE_PLUGINS, referrers_setting_,
                    brave_shields::kReferrers);
  SetContentSetting(map, kAddFundsUrl, GURL(),
                    CONTENT_SETTINGS_TYPE_PLUGINS, cookies_setting_,
                    brave_shields::kCookies);
  SetContentSetting(map, kAddFundsUrl, kFirstPartyUrl,
                    CONTENT_SETTINGS_TYPE_PLUGINS, cookies_setting_fp_,
                    brave_shields::kCookies);
}

void PopupContentSettingsBase::ResetShieldsScripts(
    HostContentSettingsMap* map) {
  SetContentSetting(map, kAddFundsUrl, GURL(), CONTENT_SETTINGS_TYPE_JAVASCRIPT,
                    js_brave_, std::string());
  SetContentSetting(map, kUpholdUrl, GURL(), CONTENT_SETTINGS_TYPE_JAVASCRIPT,
                    js_uphold_, std::string(), true);
  SetContentSetting(map, kNetverifyUrl, GURL(),
                    CONTENT_SETTINGS_TYPE_JAVASCRIPT, js_netverify_,
                    std::string(), true);
  SetContentSetting(map, kTypekitUrl, GURL(), CONTENT_SETTINGS_TYPE_JAVASCRIPT,
                    js_typekit_, std::string());
}

void PopupContentSettingsBase::ResetCameraAccess(HostContentSettingsMap* map) {
  ResetContentSetting(map, kAddFundsUrl,
                      CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA,
                      camera_setting_);
}

void PopupContentSettingsBase::ResetAutoplay(HostContentSettingsMap* map) {
  ResetContentSetting(map, kAddFundsUrl, CONTENT_SETTINGS_TYPE_AUTOPLAY,
                      autoplay_setting_);
}

ContentSetting PopupContentSettingsBase::SetContentSetting(
    HostContentSettingsMap* map,
    const GURL& host,
    const GURL& secondary,
    ContentSettingsType type,
    ContentSetting setting,
    const std::string& resource_identifier,
    bool include_host_subdomains) {
  DCHECK(map);

  // Get the current setting.
  ContentSetting current_setting =
      map->GetContentSetting(host, secondary, type, resource_identifier);

  // Check if the setting is already want we want it to be.
  if (current_setting != setting) {
    // For PLUGINS and JAVASCRIPT types, construct patterns and use custom
    // scope.
    if (type == CONTENT_SETTINGS_TYPE_PLUGINS ||
        type == CONTENT_SETTINGS_TYPE_JAVASCRIPT) {
      const ContentSettingsPattern pattern =
          GURLToPattern(host, include_host_subdomains);
      ContentSettingsPattern pattern_secondary =
          ContentSettingsPattern::Wildcard();
      if (!secondary.is_empty())
        pattern_secondary =  GURLToPattern(secondary);
      map->SetContentSettingCustomScope(pattern, pattern_secondary, type,
                                        resource_identifier, setting);
    } else {
      // For other types use default scope.
      ContentSetting default_setting =
          map->GetDefaultContentSetting(type, nullptr);
      if (current_setting == default_setting)
        current_setting = CONTENT_SETTING_DEFAULT;
      map->SetContentSettingDefaultScope(host, secondary, type,
                                         resource_identifier, setting);
    }
  }

  return current_setting;
}

void PopupContentSettingsBase::ResetContentSetting(HostContentSettingsMap* map,
                                                   const GURL& host,
                                                   ContentSettingsType type,
                                                   ContentSetting setting) {
  DCHECK(map);
  DCHECK(type != CONTENT_SETTINGS_TYPE_PLUGINS);

  if (setting == CONTENT_SETTING_DEFAULT ||
      setting != map->GetContentSetting(host, GURL(), type, std::string())) {
    map->SetContentSettingDefaultScope(host, GURL(), type, std::string(),
                                       setting);
  }
}

ContentSettingsPattern PopupContentSettingsBase::GURLToPattern(
    const GURL& gurl,
    bool wildcard_subdomains) {
  ContentSettingsPattern pattern;
  DCHECK(!gurl.is_empty());
  if (gurl.is_empty())
    return pattern;

  std::unique_ptr<ContentSettingsPattern::BuilderInterface> builder =
      ContentSettingsPattern::CreateBuilder();
  DCHECK(gurl.scheme() == url::kHttpsScheme);
  builder->WithScheme(gurl.scheme());
  builder->WithHost(gurl.host());
  if (wildcard_subdomains)
    builder->WithDomainWildcard();
  builder->WithPort("443");
  builder->WithPathWildcard();

  pattern = builder->Build();
  DCHECK(pattern.IsValid());
  return pattern;
}

}  // namespace

namespace brave_rewards {

// Pass-through to PopupContentSettingsBase which is in an anonymous namespace.
class AddFundsPopupContentSettings : public PopupContentSettingsBase {
 public:
  explicit AddFundsPopupContentSettings(Profile* profile)
      : PopupContentSettingsBase(profile) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(AddFundsPopupContentSettings);
};

AddFundsPopup::AddFundsPopup()
    : add_funds_popup_(nullptr),
      popup_content_settings_(nullptr),
      rewards_service_(nullptr),
      weak_factory_(this) {}

AddFundsPopup::~AddFundsPopup() {
  ClosePopup();
}

// Show existing or open a new popup.
void AddFundsPopup::ShowPopup(content::WebContents* initiator,
                              RewardsService* rewards_service) {
  if (add_funds_popup_) {
    Focus();
    return;
  }

  DCHECK(initiator);
  DCHECK(rewards_service);
  if (!initiator || !rewards_service)
    return;
  // Stash pointers and get addresses.
  rewards_service_ = rewards_service;
  initiator_ = initiator;

  rewards_service_->GetAddresses(base::Bind(
      &AddFundsPopup::OnGetAddresses, weak_factory_.GetWeakPtr()));
}

void AddFundsPopup::OnGetAddresses(
    const std::map<std::string, std::string>& addresses) {
  OpenPopup(addresses);
}

void AddFundsPopup::OpenPopup(
    const std::map<std::string, std::string>& addresses) {
  DCHECK(!add_funds_popup_);
  if (addresses.empty())
    return;

  DCHECK(initiator_);
  content::WebContentsDelegate* wc_delegate = initiator_->GetDelegate();
  if (!wc_delegate)
    return;

  const GURL gurl(std::string(kAddFundsPopupUrl) + "?" +
                  ToQueryString(GetAddressesAsJSON(addresses)));

  content::OpenURLParams params(gurl, content::Referrer(),
                                WindowOpenDisposition::NEW_POPUP,
                                ui::PAGE_TRANSITION_LINK, false);

  // Let popup content bypass shields, use camera and autoplay.
  std::unique_ptr<AddFundsPopupContentSettings> popup_content_settings =
      EnsureContentPermissions(initiator_);

  // Open the popup.
  add_funds_popup_ = wc_delegate->OpenURLFromTab(initiator_, params);
  DCHECK(add_funds_popup_);
  if (!add_funds_popup_)
    return;

  // We need to know when the popup closes.
  views::Widget* topLevelWidget = views::Widget::GetTopLevelWidgetForNativeView(
      add_funds_popup_->GetNativeView());
  if (!topLevelWidget) {
    // If we can't add an observer won't be able to reset when the popup closes
    // and generally this is not a good sign, so don't bother with the popup.
    ClosePopup();
    return;
  }
  topLevelWidget->AddObserver(this);

  // Keep track of the settings we changed.
  popup_content_settings_ = std::move(popup_content_settings);

  // Reposition/resize the new popup and hide actions.
  gfx::Rect popup_bounds = CalculatePopupWindowBounds(initiator_);
  topLevelWidget->SetBounds(popup_bounds);
  HideBraveActions();
  Focus();
}

void AddFundsPopup::ClosePopup() {
  if (!add_funds_popup_)
    return;

  views::Widget* widget = views::Widget::GetTopLevelWidgetForNativeView(
      add_funds_popup_->GetNativeView());
  if (widget)
    widget->RemoveObserver(this);

  add_funds_popup_->ClosePage();
  add_funds_popup_ = nullptr;
}

void AddFundsPopup::Focus() {
  DCHECK(add_funds_popup_);
  content::WebContentsDelegate* popup_delegate =
      add_funds_popup_->GetDelegate();
  if (popup_delegate)
    popup_delegate->ActivateContents(add_funds_popup_);
  add_funds_popup_->Focus();
}

// content::WidgetObserver implementation.
void AddFundsPopup::OnWidgetClosing(views::Widget* widget) {
  widget->RemoveObserver(this);
  popup_content_settings_.reset(nullptr);
  add_funds_popup_ = nullptr;
  if (rewards_service_)
    rewards_service_->FetchWalletProperties();
}

std::string AddFundsPopup::GetAddressesAsJSON(
    const std::map<std::string, std::string>& addresses) {
  // Create a dictionary of addresses for serialization.
  auto addresses_dictionary = std::make_unique<base::DictionaryValue>();
  for (const auto& pair : addresses) {
    auto address = std::make_unique<base::DictionaryValue>();
    address->SetString("address", pair.second);
    address->SetString("currency", pair.first);
    DCHECK(kCurrencyToNetworkMap.count(pair.first));
    address->SetString("network", kCurrencyToNetworkMap.count(pair.first)
                                      ? kCurrencyToNetworkMap.at(pair.first)
                                      : "");
    addresses_dictionary->SetDictionary(pair.first, std::move(address));
  }

  std::string data;
  base::JSONWriter::Write(*addresses_dictionary, &data);
  return data;
}

std::string AddFundsPopup::ToQueryString(const std::string& data) {
  std::string query_string_value;
  base::Base64Encode(data, &query_string_value);
  return ("addresses=" + net::EscapeUrlEncodedData(query_string_value, false));
}

gfx::Rect AddFundsPopup::CalculatePopupWindowBounds(WebContents* initiator) {
  // Get bounds of the initiator content to see if they would fit the
  // preferred size of our popup.
  WebContents* outermost_web_contents =
      guest_view::GuestViewBase::GetTopLevelWebContents(initiator);
  gfx::Rect initiator_bounds = outermost_web_contents->GetContainerBounds();

  gfx::Point center = initiator_bounds.CenterPoint();
  gfx::Rect popup_bounds(center.x() - kPopupPreferredWidth / 2,
                         center.y() - kPopupPreferredHeight / 2,
                         kPopupPreferredWidth, kPopupPreferredHeight);
  // Popup fits within the initiator, return the bounds no matter where the
  // initiator is on the screen.
  if (initiator_bounds.Contains(popup_bounds))
    return popup_bounds;

  // Move the popup to the center of the screen if it ended up off screen.
  // If the initiator is split between multiple displays this will show the
  // popup on the display that contains the largest chunk of the initiator
  // window. If the popup is too big for the screen, shrink it to fit.
  display::Display display =
      display::Screen::GetScreen()->GetDisplayNearestView(
          outermost_web_contents->GetNativeView());
  if (!display.bounds().IsEmpty() && !display.bounds().Contains(popup_bounds)) {
    popup_bounds = display.bounds();
    popup_bounds.ClampToCenteredSize(
        gfx::Size(kPopupPreferredWidth, kPopupPreferredHeight));
  }

  return popup_bounds;
}

std::unique_ptr<AddFundsPopupContentSettings>
AddFundsPopup::EnsureContentPermissions(content::WebContents* initiator) {
  DCHECK(initiator);
  // Get contents settings map for the current profile.
  Profile* profile =
      Profile::FromBrowserContext(initiator->GetBrowserContext());
  DCHECK(profile && !profile->IsOffTheRecord());
  return std::make_unique<AddFundsPopupContentSettings>(profile);
}

void AddFundsPopup::HideBraveActions() {
  if (!add_funds_popup_)
    return;

  Browser* browser = chrome::FindBrowserWithWebContents(add_funds_popup_);
  if (!browser)
    return;

  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  if (!browser_view)
    return;

  LocationBarView* location_bar_view = browser_view->GetLocationBarView();
  if (!location_bar_view)
    return;

  BraveLocationBarView* brave_location_bar_view =
      location_bar_view->GetBraveLocationBarView();
  if (!brave_location_bar_view)
    return;

  brave_location_bar_view->HideBraveActionsContainer();
}

}  // namespace brave_rewards
