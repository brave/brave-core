/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_BROWSER_STATE_CHROME_BROWSER_STATE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_BROWSER_STATE_CHROME_BROWSER_STATE_IMPL_H_

#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "net/url_request/url_request_job_factory.h"

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace sync_preferences {
class PrefServiceSyncable;
}

class BrowserStatePolicyConnector;
class PrefProxyConfigTracker;
class SequencedTaskRunner;
class SigninClient;

class ChromeBrowserStateImpl : public ChromeBrowserState {
 public:
  explicit ChromeBrowserStateImpl(
      scoped_refptr<base::SequencedTaskRunner> io_task_runner,
      const base::FilePath& path);
  ~ChromeBrowserStateImpl() override;

  // static ChromeBrowserStateImpl* FromBrowserState(BrowserState* browser_state);

  // net::URLRequestContextGetter* GetRequestContext() override;
  bool IsOffTheRecord() const override;
  base::FilePath GetStatePath() const override;
  ChromeBrowserState* GetOriginalChromeBrowserState() override;
  bool HasOffTheRecordChromeBrowserState() const override;
  ChromeBrowserState* GetOffTheRecordChromeBrowserState() override;
  void DestroyOffTheRecordChromeBrowserState() override;
  BrowserStatePolicyConnector* GetPolicyConnector() override;
  PrefService* GetPrefs() override;
  PrefService* GetOffTheRecordPrefs() override;
  ChromeBrowserStateIOData* GetIOData() override;
  void ClearNetworkingHistorySince(base::Time time,
                                   const base::Closure& completion) override;
  PrefProxyConfigTracker* GetProxyConfigTracker() override;
  net::URLRequestContextGetter* CreateRequestContext(
      ProtocolHandlerMap* protocol_handlers) override;

  SigninClient* GetSigninClient();

 private:
  base::FilePath state_path_;
  // scoped_refptr<base::SequencedTaskRunner> io_task_runner_;
  scoped_refptr<user_prefs::PrefRegistrySyncable> pref_registry_;
  std::unique_ptr<sync_preferences::PrefServiceSyncable> prefs_;
  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;
  std::unique_ptr<SigninClient> signin_client_;


  DISALLOW_COPY_AND_ASSIGN(ChromeBrowserStateImpl);
};

#endif /* BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_BROWSER_STATE_CHROME_BROWSER_STATE_IMPL_H_ */
