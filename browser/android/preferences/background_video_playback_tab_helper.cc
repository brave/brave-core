/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/preferences/background_video_playback_tab_helper.h"

#include <string>

#include "base/strings/utf_string_conversions.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/browser/navigation_handle.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

namespace {
const char k_youtube_background_playback_script[] =
    "(function() {"
    "    if (document._addEventListener === undefined) {"
    "        document._addEventListener = document.addEventListener;"
    "        document.addEventListener = function(a,b,c) {"
    "            if(a != 'visibilitychange') {"
    "                document._addEventListener(a,b,c);"
    "            }"
    "        };"
    "    }"
    "}());";

const char k_youtube_ads_block_script[] =
    "(function() {"
    "    const prunePaths = ['playerResponse.adPlacements',"
    "        'playerResponse.playerAds', 'adPlacements', 'playerAds'];"
    "    const findOwner = function(root, path) {"
    "        let owner = root;"
    "        let chain = path;"
    "        for (;;) {"
    "            if ( owner instanceof Object === false ) { return; }"
    "            const pos = chain.indexOf('.');"
    "            if ( pos === -1 ) {"
    "                return owner.hasOwnProperty(chain)? [ owner, chain ]:"
    "                    undefined;"
    "            }"
    "            const prop = chain.slice(0, pos);"
    "            if ( owner.hasOwnProperty(prop) === false ) { return; }"
    "            owner = owner[prop];"
    "            chain = chain.slice(pos + 1);"
    "        }"
    "    };"
    "    JSON.parse = new Proxy(JSON.parse, {"
    "        apply: function() {"
    "            const r = Reflect.apply(...arguments);"
    "            for ( const path of prunePaths ) {"
    "                const details = findOwner(r, path);"
    "                if ( details !== undefined ) {"
    "                    delete details[0][details[1]];"
    "                }"
    "            }"
    "            return r;"
    "        },"
    "    });"
    "})();";

const char k_archive_ads_block_script[] =
    "(function() {"
      "const target = 'Object.getPrototypeOf';"
      "if ( target === '' || target === '{{1}}' ) { return; }"
      "const needle = 'join';"
      "let reText = '.?';"
      "if ( needle !== '' && needle !== '{{2}}' ) {"
          "reText = /^\\/.+\\/$/.test(needle)"
              "? needle.slice(1,-1)"
              ": needle.replace(/[.*+?^${}()|[\\]\\\\]/g, '\\\\$&');"
      "}"
      "const thisScript = document.currentScript;"
      "const re = new RegExp(reText);"
      "const chain = target.split('.');"
      "let owner = window;"
      "let prop;"
      "for (;;) {"
          "prop = chain.shift();"
          "if ( chain.length === 0 ) { break; }"
          "owner = owner[prop];"
          "if ( owner instanceof Object === false ) { return; }"
      "}"
      "let value;"
      "let desc = Object.getOwnPropertyDescriptor(owner, prop);"
      "if ("
          "desc instanceof Object === false ||"
          "desc.get instanceof Function === false"
      ") {"
          "value = owner[prop];"
          "desc = undefined;"
      "}"
      "const magic = String.fromCharCode(Date.now() % 26 + 97) +"
                    "Math.floor(Math.random() * "
                    "982451653 + 982451653).toString(36);"
      "const validate = function() {"
          "const e = document.currentScript;"
          "if ("
              "e instanceof HTMLScriptElement &&"
              "e.src === '' &&"
              "e !== thisScript &&"
              "re.test(e.textContent)"
          ") {"
              "throw new ReferenceError(magic);"
          "}"
      "};"
      "Object.defineProperty(owner, prop, {"
          "get: function() {"
              "validate();"
              "return desc instanceof Object"
                  "? desc.get()"
                  ": value;"
          "},"
          "set: function(a) {"
              "validate();"
              "if ( desc instanceof Object ) {"
                  "desc.set(a);"
              "} else {"
                  "value = a;"
              "}"
          "}"
      "});"
      "const oe = window.onerror;"
      "window.onerror = function(msg) {"
          "if ( typeof msg === 'string' && msg.indexOf(magic) !== -1 ) {"
              "return true;"
          "}"
          "if ( oe instanceof Function ) {"
              "return oe.apply(this, arguments);"
          "}"
      "}.bind();"
    "})();"
    "(function() {"
      "const target = 'document.cookie';"
      "if ( target === '' || target === '{{1}}' ) { return; }"
      "const needle = '{{2}}';"
      "let reText = '.?';"
      "if ( needle !== '' && needle !== '{{2}}' ) {"
          "reText = /^\\/.+\\/$/.test(needle)"
              "? needle.slice(1,-1)"
              ": needle.replace(/[.*+?^${}()|[\\]\\\\]/g, '\\\\$&');"
      "}"
      "const thisScript = document.currentScript;"
      "const re = new RegExp(reText);"
      "const chain = target.split('.');"
      "let owner = window;"
      "let prop;"
      "for (;;) {"
          "prop = chain.shift();"
          "if ( chain.length === 0 ) { break; }"
          "owner = owner[prop];"
          "if ( owner instanceof Object === false ) { return; }"
      "}"
      "let value;"
      "let desc = Object.getOwnPropertyDescriptor(owner, prop);"
      "if ("
          "desc instanceof Object === false ||"
          "desc.get instanceof Function === false"
      ") {"
          "value = owner[prop];"
          "desc = undefined;"
      "}"
      "const magic = String.fromCharCode(Date.now() % 26 + 97) +"
                    "Math.floor(Math.random() * "
                    "982451653 + 982451653).toString(36);"
      "const validate = function() {"
          "const e = document.currentScript;"
          "if ("
              "e instanceof HTMLScriptElement &&"
              "e.src === '' &&"
              "e !== thisScript &&"
              "re.test(e.textContent)"
          ") {"
              "throw new ReferenceError(magic);"
          "}"
      "};"
      "Object.defineProperty(owner, prop, {"
          "get: function() {"
              "validate();"
              "return desc instanceof Object"
                  "? desc.get()"
                  ": value;"
          "},"
          "set: function(a) {"
              "validate();"
              "if ( desc instanceof Object ) {"
                  "desc.set(a);"
              "} else {"
                  "value = a;"
              "}"
          "}"
      "});"
      "const oe = window.onerror;"
      "window.onerror = function(msg) {"
          "if ( typeof msg === 'string' && msg.indexOf(magic) !== -1 ) {"
              "return true;"
          "}"
          "if ( oe instanceof Function ) {"
              "return oe.apply(this, arguments);"
          "}"
      "}.bind();"
    "})();"
    "(function() {"
      "const magic = String.fromCharCode(Date.now() % 26 + 97) +"
      "              Math.floor(Math.random() * "
      "                  982451653 + 982451653).toString(36);"
      "let prop = 'navigator.brave';"
      "let owner = window;"
      "for (;;) {"
      "    const pos = prop.indexOf('.');"
      "    if ( pos === -1 ) { break; }"
      "    owner = owner[prop.slice(0, pos)];"
      "    if ( owner instanceof Object === false ) { return; }"
      "    prop = prop.slice(pos + 1);"
      "}"
      "delete owner[prop];"
      "Object.defineProperty(owner, prop, {"
      "    set: function() {"
      "        throw new ReferenceError(magic);"
      "    }"
      "});"
      "const oe = window.onerror;"
      "window.onerror = function(msg, src, line, col, error) {"
      "    if ( typeof msg === 'string' && msg.indexOf(magic) !== -1 ) {"
      "        return true;"
      "    }"
      "    if ( oe instanceof Function ) {"
      "        return oe(msg, src, line, col, error);"
      "    }"
      "}.bind();"
  "})();"
  "(function() {"
      "const target = 'navigator.brave';"
      "if ( target === '' || target === '{{1}}' ) { return; }"
      "const needle = '{{2}}';"
      "let reText = '.?';"
      "if ( needle !== '' && needle !== '{{2}}' ) {"
      "    reText = /^\\/.+\\/$/.test(needle)"
      "        ? needle.slice(1,-1)"
      "        : needle.replace(/[.*+?^${}()|[\\]\\\\]/g, '\\\\$&');"
      "}"
      "const thisScript = document.currentScript;"
      "const re = new RegExp(reText);"
      "const chain = target.split('.');"
      "let owner = window;"
      "let prop;"
      "for (;;) {"
      "    prop = chain.shift();"
      "    if ( chain.length === 0 ) { break; }"
      "    owner = owner[prop];"
      "    if ( owner instanceof Object === false ) { return; }"
      "}"
      "let value;"
      "let desc = Object.getOwnPropertyDescriptor(owner, prop);"
      "if ("
      "    desc instanceof Object === false ||"
      "    desc.get instanceof Function === false"
      ") {"
      "    value = owner[prop];"
      "    desc = undefined;"
      "}"
      "const magic = String.fromCharCode(Date.now() % 26 + 97) + "
      "              Math.floor(Math.random() * 982451653 + "
      "                 982451653).toString(36);"
      "const validate = function() {"
      "    const e = document.currentScript;"
      "    if ("
      "        e instanceof HTMLScriptElement &&"
      "        e.src === '' &&"
      "        e !== thisScript &&"
      "        re.test(e.textContent)"
      "    ) {"
      "        throw new ReferenceError(magic);"
      "    }"
      "};"
      "Object.defineProperty(owner, prop, {"
      "    get: function() {"
      "        validate();"
      "        return desc instanceof Object"
      "            ? desc.get()"
      "            : value;"
      "    },"
      "    set: function(a) {"
      "        validate();"
      "        if ( desc instanceof Object ) {"
      "            desc.set(a);"
      "        } else {"
      "            value = a;"
      "        }"
      "    }"
      "});"
      "document.cookie = 'unsupported-browser=;"
        " expires=Thu, 01 Jan 1970 00:00:01 GMT';"
      "const oe = window.onerror;"
      "window.onerror = function(msg) {"
      "    if ( typeof msg === 'string' && msg.indexOf(magic) !== -1 ) {"
      "        return true;"
      "    }"
      "    if ( oe instanceof Function ) {"
      "        return oe.apply(this, arguments);"
      "    }"
      "}.bind();"
  "})();";

bool IsYouTubeDomain(const GURL& url) {
  if (net::registry_controlled_domains::SameDomainOrHost(
          url, GURL("https://www.youtube.com"),
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    return true;
  }

  return false;
}

bool IsArchiveDomain(const GURL& url) {
  if (net::registry_controlled_domains::SameDomainOrHost(
          url, GURL("https://archive.is"),
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES) ||
      net::registry_controlled_domains::SameDomainOrHost(
          url, GURL("https://archive.today"),
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES) ||
      net::registry_controlled_domains::SameDomainOrHost(
          url, GURL("https://archive.vn"),
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES) ||
      net::registry_controlled_domains::SameDomainOrHost(
          url, GURL("https://archive.fo"),
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    return true;
  }

  return false;
}

bool IsBackgroundVideoPlaybackEnabled(content::WebContents* contents) {
  PrefService* prefs =
      static_cast<Profile*>(contents->GetBrowserContext())->GetPrefs();
  if (!prefs->GetBoolean(kBackgroundVideoPlaybackEnabled))
    return false;

  content::RenderFrameHost::AllowInjectingJavaScript();

  return true;
}

bool IsAdBlockEnabled(content::WebContents* contents, const GURL& url) {
  Profile* profile = static_cast<Profile*>(contents->GetBrowserContext());

  auto* map = HostContentSettingsMapFactory::GetForProfile(profile);
  brave_shields::ControlType control_type =
      brave_shields::GetAdControlType(map, contents->GetLastCommittedURL());
  if (brave_shields::GetBraveShieldsEnabled(map, url) &&
      control_type != brave_shields::ALLOW) {
    content::RenderFrameHost::AllowInjectingJavaScript();

    return true;
  }

  return false;
}

}  // namespace

BackgroundVideoPlaybackTabHelper::BackgroundVideoPlaybackTabHelper(
    content::WebContents* contents)
    : WebContentsObserver(contents) {
}

BackgroundVideoPlaybackTabHelper::~BackgroundVideoPlaybackTabHelper() {
}

void BackgroundVideoPlaybackTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  // Filter only YT and archive domains here
  bool yt_domain = IsYouTubeDomain(web_contents()->GetLastCommittedURL());
  bool archive_domain = IsArchiveDomain(web_contents()->GetLastCommittedURL());
  if (!yt_domain && !archive_domain) {
    return;
  }
  if (IsBackgroundVideoPlaybackEnabled(web_contents())) {
    web_contents()->GetMainFrame()->ExecuteJavaScript(
        base::UTF8ToUTF16(k_youtube_background_playback_script),
        base::NullCallback());
  }
  if (IsAdBlockEnabled(web_contents(),
      web_contents()->GetLastCommittedURL())) {
    if (yt_domain) {
      web_contents()->GetMainFrame()->ExecuteJavaScript(
          base::UTF8ToUTF16(k_youtube_ads_block_script),
          base::NullCallback());
    } else if (archive_domain) {
      web_contents()->GetMainFrame()->ExecuteJavaScript(
          base::UTF8ToUTF16(k_archive_ads_block_script),
          base::NullCallback());
    }
  }
}

void BackgroundVideoPlaybackTabHelper::ResourceLoadComplete(
    content::RenderFrameHost* render_frame_host,
    const content::GlobalRequestID& request_id,
    const blink::mojom::ResourceLoadInfo& resource_load_info) {
  bool yt_domain = IsYouTubeDomain(resource_load_info.final_url);
  bool archive_domain = IsArchiveDomain(resource_load_info.final_url);
  if (!render_frame_host || (!yt_domain && !archive_domain) ||
      !IsAdBlockEnabled(web_contents(), resource_load_info.final_url)) {
    return;
  }

  if (yt_domain) {
    render_frame_host->ExecuteJavaScript(
        base::UTF8ToUTF16(k_youtube_ads_block_script),
        base::NullCallback());
  } else if (archive_domain) {
    render_frame_host->ExecuteJavaScript(
        base::UTF8ToUTF16(k_archive_ads_block_script),
        base::NullCallback());
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BackgroundVideoPlaybackTabHelper)
