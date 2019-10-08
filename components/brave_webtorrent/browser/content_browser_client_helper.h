/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/task/post_task.h"
#include "brave/common/url_constants.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/components/brave_webtorrent/browser/webtorrent_util.h"
#include "chrome/browser/external_protocol/external_protocol_handler.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_set.h"
#include "net/base/escape.h"

namespace webtorrent {

static GURL TranslateMagnetURL(const GURL& url) {
  GURL extension_page_url(
      base::StrCat({extensions::kExtensionScheme, "://",
        brave_webtorrent_extension_id,
        "/extension/brave_webtorrent.html?%s"}));
  std::string translatedSpec(extension_page_url.spec());
  base::ReplaceFirstSubstringAfterOffset(
      &translatedSpec, 0, "%s",
      net::EscapeQueryParamValue(url.spec(), true));
  return GURL(translatedSpec);
}

static GURL TranslateTorrentUIURLReversed(const GURL& url) {
  GURL translatedURL(net::UnescapeURLComponent(
        url.query(), net::UnescapeRule::URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS |
        net::UnescapeRule::PATH_SEPARATORS));
  GURL::Replacements replacements;
  replacements.SetRefStr(url.ref_piece());
  return translatedURL.ReplaceComponents(replacements);
}

static bool HandleTorrentURLReverseRewrite(GURL* url,
    content::BrowserContext* browser_context) {
  if (url->SchemeIs(extensions::kExtensionScheme) &&
      url->host() == brave_webtorrent_extension_id &&
      url->ExtractFileName() == "brave_webtorrent.html") {
    *url =  TranslateTorrentUIURLReversed(*url);
    return true;
  }

  return false;
}

static bool HandleTorrentURLRewrite(GURL* url,
    content::BrowserContext* browser_context) {
  if (!IsWebtorrentEnabled(browser_context)) return false;

  // The HTTP/HTTPS URL could be modified later by the network delegate if the
  // mime type matches or .torrent is in the path.
  // Handle http and https here for making reverse_on_redirect to be true in
  // BrowserURLHandlerImpl::RewriteURLIfNecessary to trigger ReverseURLRewrite
  // for updating the virtual URL.
  if (url->SchemeIsHTTPOrHTTPS() ||
      (url->SchemeIs(extensions::kExtensionScheme) &&
       url->host() == brave_webtorrent_extension_id &&
       url->ExtractFileName() == "brave_webtorrent.html")) {
    return true;
  }

  return false;
}

static void LoadOrLaunchMagnetURL(
    const GURL& url,
    const content::WebContents::Getter& web_contents_getter,
    ui::PageTransition page_transition,
    bool has_user_gesture) {
  content::WebContents* web_contents = web_contents_getter.Run();
  if (!web_contents)
    return;

  if (IsWebtorrentEnabled(web_contents->GetBrowserContext())) {
    web_contents->GetController().LoadURL(url, content::Referrer(),
        page_transition, std::string());
  } else {
    ExternalProtocolHandler::LaunchUrl(
        url, web_contents->GetRenderViewHost()->GetProcess()->GetID(),
        web_contents->GetRenderViewHost()->GetRoutingID(), page_transition,
        has_user_gesture);
  }
}

static bool HandleMagnetURLRewrite(GURL* url,
    content::BrowserContext* browser_context) {
  if (IsWebtorrentEnabled(browser_context) && url->SchemeIs(kMagnetScheme)) {
    *url = TranslateMagnetURL(*url);
    return true;
  }

  return false;
}

static bool HandleMagnetProtocol(
    const GURL& url,
    content::WebContents::Getter web_contents_getter,
    ui::PageTransition page_transition,
    bool has_user_gesture) {
  if (url.SchemeIs(kMagnetScheme)) {
    base::PostTaskWithTraits(FROM_HERE, {content::BrowserThread::UI},
        base::BindOnce(&LoadOrLaunchMagnetURL, url, web_contents_getter,
        page_transition, has_user_gesture));
    return true;
  }

  return false;
}

}
