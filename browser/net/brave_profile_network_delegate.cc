/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_profile_network_delegate.h"

#include <string>

#include "base/task/post_task.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/net/brave_ad_block_tp_network_delegate_helper.h"
#include "brave/browser/net/brave_common_static_redirect_network_delegate_helper.h"
#include "brave/browser/net/brave_httpse_network_delegate_helper.h"
#include "brave/browser/net/brave_site_hacks_network_delegate_helper.h"
#include "brave/browser/translate/buildflags/buildflags.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_referrals/buildflags/buildflags.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_shields/browser/ad_block_custom_filters_service.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
#include "brave/browser/net/brave_referrals_network_delegate_helper.h"
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/components/brave_rewards/browser/net/network_delegate_helper.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
#include "brave/components/brave_webtorrent/browser/net/brave_torrent_redirect_network_delegate_helper.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
#include "brave/browser/net/brave_translate_redirect_network_delegate_helper.h"
#endif

using content::BrowserThread;

BraveProfileNetworkDelegate::BraveProfileNetworkDelegate(
    extensions::EventRouterForwarder* event_router)
    : BraveNetworkDelegateBase(event_router),
      weak_ptr_factory_io_(this),
      weak_ptr_factory_ui_(this) {
  brave::OnBeforeURLRequestCallback
  callback =
      base::Bind(brave::OnBeforeURLRequest_SiteHacksWork);
  before_url_request_callbacks_.push_back(callback);

  callback =
      base::Bind(brave::OnBeforeURLRequest_AdBlockTPPreWork);
  before_url_request_callbacks_.push_back(callback);

  callback =
      base::Bind(brave::OnBeforeURLRequest_HttpsePreFileWork);
  before_url_request_callbacks_.push_back(callback);

  callback =
      base::Bind(brave::OnBeforeURLRequest_CommonStaticRedirectWork);
  before_url_request_callbacks_.push_back(callback);

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  callback = base::Bind(brave_rewards::OnBeforeURLRequest);
  before_url_request_callbacks_.push_back(callback);
#endif

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
  callback = base::BindRepeating(
      brave::OnBeforeURLRequest_TranslateRedirectWork);
  before_url_request_callbacks_.push_back(callback);
#endif

  brave::OnBeforeStartTransactionCallback start_transaction_callback =
      base::Bind(brave::OnBeforeStartTransaction_SiteHacksWork);
  before_start_transaction_callbacks_.push_back(start_transaction_callback);

#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
  start_transaction_callback =
      base::Bind(brave::OnBeforeStartTransaction_ReferralsWork);
  before_start_transaction_callbacks_.push_back(start_transaction_callback);
#endif

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
  brave::OnHeadersReceivedCallback headers_received_callback =
      base::Bind(
          webtorrent::OnHeadersReceived_TorrentRedirectWork);
  headers_received_callbacks_.push_back(headers_received_callback);
#endif

  // Initialize the preference change registrar.
  // call PostTask because we need to wait for profile_path to be set
  // on the OI thread after BraveProfileNetworkDelegate is created
  base::PostTaskWithTraits(
      FROM_HERE, {BrowserThread::IO},
      base::Bind(&BraveProfileNetworkDelegate::InitPrefChangeRegistrar,
                 weak_ptr_factory_ui_.GetWeakPtr()));
}

BraveProfileNetworkDelegate::~BraveProfileNetworkDelegate() {
}

void BraveProfileNetworkDelegate::InitPrefChangeRegistrar() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(!profile_path().empty());

  // profile path is set on the IO thread so we need to read it here and
  // then pass the value to the UI thread
  base::PostTaskWithTraits(
      FROM_HERE, {BrowserThread::UI},
      base::Bind(&BraveProfileNetworkDelegate::InitPrefChangeRegistrarOnUI,
                 weak_ptr_factory_io_.GetWeakPtr(), profile_path()));
}

void BraveProfileNetworkDelegate::InitPrefChangeRegistrarOnUI(
    const base::FilePath& profile_path) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto* profile =
      g_browser_process->profile_manager()->GetProfile(profile_path);

  PrefService* user_prefs = profile->GetPrefs();
  user_pref_change_registrar_.reset(new PrefChangeRegistrar());
  user_pref_change_registrar_->Init(user_prefs);
  user_pref_change_registrar_->Add(
      kGoogleLoginControlType,
      base::BindRepeating(&BraveProfileNetworkDelegate::OnPreferenceChanged,
                          weak_ptr_factory_ui_.GetWeakPtr(),
                          user_prefs,
                          kGoogleLoginControlType));
  OnPreferenceChanged(user_prefs, kGoogleLoginControlType);
}

void BraveProfileNetworkDelegate::OnPreferenceChanged(
    PrefService* user_prefs,
    const std::string& pref_name) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (pref_name == kGoogleLoginControlType) {
    base::PostTaskWithTraits(
        FROM_HERE, {BrowserThread::IO},
        base::Bind(&BraveProfileNetworkDelegate::UpdateGoogleAuthOnIO,
                   weak_ptr_factory_ui_.GetWeakPtr(),
                   user_prefs->GetBoolean(kGoogleLoginControlType)));
  }
}

void BraveProfileNetworkDelegate::UpdateGoogleAuthOnIO(bool allow_goole_auth) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  set_allow_google_auth(allow_goole_auth);
}
