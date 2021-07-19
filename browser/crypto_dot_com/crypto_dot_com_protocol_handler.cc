/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/crypto_dot_com/crypto_dot_com_protocol_handler.h"

#include <string>
#include <utility>

#include "base/task/post_task.h"
#include "brave/browser/crypto_dot_com/crypto_dot_com_service_factory.h"
#include "brave/common/url_constants.h"
#include "brave/components/crypto_dot_com/browser/crypto_dot_com_service.h"
#include "brave/components/crypto_dot_com/common/constants.h"
#include "chrome/common/webui_url_constants.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

namespace {

std::string GetTokenFromURL(const GURL& url) {
  constexpr char kTokenKey[] = "token";
  std::string token;
  net::GetValueForKeyInQuery(url, kTokenKey, &token);
  return token;
}

void LoadNewTabURL(const GURL& url,
                   content::WebContents::OnceGetter web_contents_getter,
                   ui::PageTransition page_transition,
                   bool has_user_gesture,
                   const base::Optional<url::Origin>& initiating_origin) {
  content::WebContents* web_contents = std::move(web_contents_getter).Run();
  if (!web_contents) {
    return;
  }

  const auto ref_url = web_contents->GetURL();
  if (!ref_url.is_valid()) {
    return;
  }

  url::Origin allowed_origin = url::Origin::Create(GURL(kCryptoDotComAuthURL));
  url::Origin newtab_origin =
      url::Origin::Create(GURL(chrome::kChromeUINewTabURL));
  const GURL last_committed_origin =
      web_contents->GetLastCommittedURL().GetOrigin();
  // When browser loads auth url again in logged state, service could gives
  // access token via redirect url with response.
  // In this case, origin is newtab.
  // After developing is finished, service will load user consent page always.
  // TODO(simonhong): Replace newtab_origin with user consent page when that
  // page is ready.
  if ((last_committed_origin != allowed_origin.GetURL() &&
       last_committed_origin != newtab_origin.GetURL()) ||
      !initiating_origin ||
      (initiating_origin.value() != allowed_origin &&
       initiating_origin.value() != newtab_origin)) {
    return;
  }

  std::string token = GetTokenFromURL(url);
  CryptoDotComServiceFactory::GetInstance()
      ->GetForProfile(web_contents->GetBrowserContext())
      ->SetAccessToken(token);

  web_contents->GetController().LoadURL(GURL(chrome::kChromeUINewTabURL),
                                        content::Referrer(), page_transition,
                                        std::string());
}

}  // namespace

namespace crypto_dot_com {

void HandleCryptoDotComProtocol(
    const GURL& url,
    content::WebContents::OnceGetter web_contents_getter,
    ui::PageTransition page_transition,
    bool has_user_gesture,
    const base::Optional<url::Origin>& initiator) {
  DCHECK(IsCryptoDotComProtocol(url));
  base::PostTask(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&LoadNewTabURL, url, std::move(web_contents_getter),
                     page_transition, has_user_gesture, initiator));
}

bool IsCryptoDotComProtocol(const GURL& url) {
  return url.SchemeIs(kCryptoDotComScheme);
}

}  // namespace crypto_dot_com
