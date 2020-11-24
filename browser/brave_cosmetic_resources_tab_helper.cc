/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_cosmetic_resources_tab_helper.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/brave_shields/browser/ad_block_custom_filters_service.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/ad_block_service_helper.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/content/browser/cosmetic_filters_communication_impl.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/renderer/render_frame.h"
#include "components/cosmetic_filters/resources/grit/cosmetic_filters_resources.h"
#include "ui/base/resource/resource_bundle.h"

// Doing that to prevent lint error:
// Static/global string variables are not permitted
// You can create an object dynamically and never delete it by using a
// Function-local static pointer or reference
// (e.g., static const auto& impl = *new T(args...);).
std::string* BraveCosmeticResourcesTabHelper::observing_script_ =
    new std::string("");

namespace {

bool ShouldDoCosmeticFiltering(content::WebContents* contents,
    const GURL& url) {
  Profile* profile = Profile::FromBrowserContext(contents->GetBrowserContext());
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile);

  return ::brave_shields::ShouldDoCosmeticFiltering(map, url);
}

std::string LoadDataResource(const int& id) {
  std::string data_resource;

  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  if (resource_bundle.IsGzipped(id)) {
    data_resource = resource_bundle.LoadDataResourceString(id);
  } else {
    data_resource = resource_bundle.GetRawDataResource(id).as_string();
  }

  return data_resource;
}

std::unique_ptr<base::ListValue> GetUrlCosmeticResourcesOnTaskRunner(
    const std::string& url) {
  auto result_list = std::make_unique<base::ListValue>();

  base::Optional<base::Value> resources = g_brave_browser_process->
      ad_block_service()->UrlCosmeticResources(url);

  if (!resources || !resources->is_dict()) {
    return result_list;
  }

  base::Optional<base::Value> regional_resources = g_brave_browser_process->
      ad_block_regional_service_manager()->UrlCosmeticResources(url);

  if (regional_resources && regional_resources->is_dict()) {
    ::brave_shields::MergeResourcesInto(
        std::move(*regional_resources), &*resources, /*force_hide=*/false);
  }

  base::Optional<base::Value> custom_resources = g_brave_browser_process->
      ad_block_custom_filters_service()->UrlCosmeticResources(url);

  if (custom_resources && custom_resources->is_dict()) {
    ::brave_shields::MergeResourcesInto(
        std::move(*custom_resources), &*resources, /*force_hide=*/true);
  }

  result_list->Append(std::move(*resources));

  return result_list;
}

std::unique_ptr<base::ListValue> GetHiddenClassIdSelectorsOnTaskRunner(
    const std::vector<std::string>& classes,
    const std::vector<std::string>& ids,
    const std::vector<std::string>& exceptions) {
  base::Optional<base::Value> hide_selectors = g_brave_browser_process->
      ad_block_service()->HiddenClassIdSelectors(classes, ids, exceptions);

  base::Optional<base::Value> regional_selectors = g_brave_browser_process->
      ad_block_regional_service_manager()->
          HiddenClassIdSelectors(classes, ids, exceptions);

  base::Optional<base::Value> custom_selectors = g_brave_browser_process->
      ad_block_custom_filters_service()->
          HiddenClassIdSelectors(classes, ids, exceptions);

  if (hide_selectors && hide_selectors->is_list()) {
    if (regional_selectors && regional_selectors->is_list()) {
      for (auto i = regional_selectors->GetList().begin();
          i < regional_selectors->GetList().end(); i++) {
        hide_selectors->Append(std::move(*i));
      }
    }
  } else {
    hide_selectors = std::move(regional_selectors);
  }

  auto result_list = std::make_unique<base::ListValue>();
  if (hide_selectors && hide_selectors->is_list()) {
    result_list->Append(std::move(*hide_selectors));
  }
  if (custom_selectors && custom_selectors->is_list()) {
    result_list->Append(std::move(*custom_selectors));
  }

  return result_list;
}

}  // namespace


BraveCosmeticResourcesTabHelper::BraveCosmeticResourcesTabHelper(
    content::WebContents* contents)
    : WebContentsObserver(contents),
    enabled_1st_party_cf_filtering_(false) {
  if (BraveCosmeticResourcesTabHelper::observing_script_->empty()) {
    *BraveCosmeticResourcesTabHelper::observing_script_ =
        LoadDataResource(IDR_COSMETIC_FILTERS_SCRIPT);
  }
}

BraveCosmeticResourcesTabHelper::~BraveCosmeticResourcesTabHelper() {
}

void BraveCosmeticResourcesTabHelper::GetUrlCosmeticResourcesOnUI(
    content::GlobalFrameRoutingId frame_id, const std::string& url,
    bool main_frame, std::unique_ptr<base::ListValue> resources) {
  if (!resources) {
    return;
  }
  for (auto i = resources->GetList().begin();
      i < resources->GetList().end(); i++) {
    base::DictionaryValue* resources_dict;
    if (!i->GetAsDictionary(&resources_dict)) {
      continue;
    }
    std::string to_inject;
    resources_dict->GetString("injected_script", &to_inject);
    if (to_inject.length() > 1) {
      auto* frame_host = content::RenderFrameHost::FromID(frame_id);
      if (!frame_host)
        return;
      frame_host->ExecuteJavaScriptInIsolatedWorld(
          base::UTF8ToUTF16(to_inject),
          base::NullCallback(), ISOLATED_WORLD_ID_CHROME_INTERNAL);
    }
    // Working on css rules
    if (!main_frame)
      return;
    CSSRulesRoutine(url, resources_dict, frame_id);
  }
}

void BraveCosmeticResourcesTabHelper::CSSRulesRoutine(
    const std::string& url_string, base::DictionaryValue* resources_dict,
    content::GlobalFrameRoutingId frame_id) {
  // Check are first party cosmetic filters enabled
  const GURL url(url_string);
  if (url.is_empty() || !url.is_valid()) {
    return;
  }

  Profile* profile = Profile::FromBrowserContext(
    web_contents()->GetBrowserContext());
  enabled_1st_party_cf_filtering_ =
      brave_shields::IsFirstPartyCosmeticFilteringEnabled(
          HostContentSettingsMapFactory::GetForProfile(profile), url);
  base::ListValue* cf_exceptions_list;
  if (resources_dict->GetList("exceptions", &cf_exceptions_list)) {
    for (size_t i = 0; i < cf_exceptions_list->GetSize(); i++) {
      exceptions_.push_back(cf_exceptions_list->GetList()[i].GetString());
    }
  }
  base::ListValue* hide_selectors_list;
  base::ListValue* force_hide_selectors_list = nullptr;
  if (resources_dict->GetList("hide_selectors", &hide_selectors_list)) {
    if (enabled_1st_party_cf_filtering_) {
      force_hide_selectors_list = hide_selectors_list;
    } else {
      std::string cosmeticFilterConsiderNewSelectors_script =
          "(function() {"
            "let nextIndex = window.cosmeticStyleSheet.rules.length;";
      for (size_t i = 0; i < hide_selectors_list->GetSize(); i++) {
        std::string rule = hide_selectors_list->GetList()[i].GetString() +
            "{display:none !important;}";
        cosmeticFilterConsiderNewSelectors_script +=
            "window.cosmeticStyleSheet.insertRule(`" + rule +
            "`,nextIndex);"
            "nextIndex++;";
      }
      cosmeticFilterConsiderNewSelectors_script +=
          "if (!document.adoptedStyleSheets.includes("
                "window.cosmeticStyleSheet)){"
              "document.adoptedStyleSheets = [window.cosmeticStyleSheet];"
          "};";
      cosmeticFilterConsiderNewSelectors_script += "})();";
      if (hide_selectors_list->GetSize() != 0) {
        auto* frame_host = content::RenderFrameHost::FromID(frame_id);
        if (!frame_host)
          return;
        frame_host->ExecuteJavaScriptInIsolatedWorld(
            base::UTF8ToUTF16(cosmeticFilterConsiderNewSelectors_script),
            base::NullCallback(), ISOLATED_WORLD_ID_CHROME_INTERNAL);
      }
    }
  }

  std::string styled_stylesheet = "";
  if (force_hide_selectors_list!= nullptr &&
      force_hide_selectors_list->GetSize() != 0) {
    for (size_t i = 0; i < force_hide_selectors_list->GetSize(); i++) {
      if (i != 0) {
        styled_stylesheet += ",";
      }
      styled_stylesheet += force_hide_selectors_list->GetList()[i].GetString();
    }
    styled_stylesheet += "{display:none!important;}\n";
  }
  base::DictionaryValue* style_selectors_dictionary = nullptr;
  if (resources_dict->GetDictionary("style_selectors",
      &style_selectors_dictionary)) {
    for (const auto& it : style_selectors_dictionary->DictItems()) {
      base::ListValue* style_selectors_list = nullptr;
      if (!style_selectors_dictionary->GetList(it.first,
          &style_selectors_list) || style_selectors_list->GetSize() == 0) {
        continue;
      }
      styled_stylesheet += it.first + "{";
      for (size_t i = 0; i < style_selectors_list->GetSize(); i++) {
        if (i != 0) {
          styled_stylesheet += ";";
        }
        styled_stylesheet += style_selectors_list->GetList()[i].GetString();
      }
      styled_stylesheet += ";}\n";
    }
    std::string cosmeticFilterConsiderNewSelectors_script =
      "(function() {";
    cosmeticFilterConsiderNewSelectors_script +=
        "window.cosmeticStyleSheet.insertRule(`" + styled_stylesheet +
        "`,window.cosmeticStyleSheet.rules.length);";
    cosmeticFilterConsiderNewSelectors_script +=
      "if (!document.adoptedStyleSheets.includes("
            "window.cosmeticStyleSheet)){"
          "document.adoptedStyleSheets = [window.cosmeticStyleSheet];"
      "};";
    cosmeticFilterConsiderNewSelectors_script += "})();";
    auto* frame_host = content::RenderFrameHost::FromID(frame_id);
    if (!frame_host)
      return;
    frame_host->ExecuteJavaScriptInIsolatedWorld(
        base::UTF8ToUTF16(cosmeticFilterConsiderNewSelectors_script),
        base::NullCallback(), ISOLATED_WORLD_ID_CHROME_INTERNAL);
  }
  //
}

void BraveCosmeticResourcesTabHelper::GetHiddenClassIdSelectorsOnUI(
    content::GlobalFrameRoutingId frame_id,
    std::unique_ptr<base::ListValue> selectors) {
  if (!selectors) {
    return;
  }
  if (enabled_1st_party_cf_filtering_) {
    // (sergz) We do the same in else section, we need to figure out do we
    // need that if at all
  } else {
    std::string cosmeticFilterConsiderNewSelectors_script =
        "(function() {"
          "let nextIndex = window.cosmeticStyleSheet.rules.length;";
    bool execute_script = false;
    for (size_t i = 0; i < selectors->GetSize(); i++) {
      base::ListValue* selectors_list = nullptr;
      if (!selectors->GetList()[i].GetAsList(&selectors_list) ||
          selectors_list->GetSize() == 0) {
        continue;
      }
      for (size_t j = 0; j < selectors_list->GetSize(); j++) {
        std::string rule = selectors_list->GetList()[i].GetString() +
            "{display:none !important;}";
        cosmeticFilterConsiderNewSelectors_script +=
            "window.cosmeticStyleSheet.insertRule(`" + rule +
            "`,nextIndex);"
            "nextIndex++;";
        execute_script = true;
      }
    }
    if (execute_script) {
      cosmeticFilterConsiderNewSelectors_script +=
          "if (!document.adoptedStyleSheets.includes("
                "window.cosmeticStyleSheet)){"
              "document.adoptedStyleSheets = [window.cosmeticStyleSheet];"
          "};";
      cosmeticFilterConsiderNewSelectors_script += "})();";
      auto* frame_host = content::RenderFrameHost::FromID(frame_id);
      if (!frame_host)
        return;
      frame_host->ExecuteJavaScriptInIsolatedWorld(
          base::UTF8ToUTF16(cosmeticFilterConsiderNewSelectors_script),
          base::NullCallback(), ISOLATED_WORLD_ID_CHROME_INTERNAL);
    }
  }
}

void BraveCosmeticResourcesTabHelper::ProcessURL(
    content::RenderFrameHost* render_frame_host, const GURL& url,
    const bool& main_frame) {
  content::CosmeticFiltersCommunicationImpl::CreateInstance(render_frame_host,
      this);
  if (!render_frame_host || !ShouldDoCosmeticFiltering(web_contents(), url)) {
    return;
  }
  g_brave_browser_process->ad_block_service()->GetTaskRunner()->
      PostTaskAndReplyWithResult(FROM_HERE,
          base::BindOnce(GetUrlCosmeticResourcesOnTaskRunner,
            url.spec()),
          base::BindOnce(&BraveCosmeticResourcesTabHelper::
            GetUrlCosmeticResourcesOnUI, AsWeakPtr(),
            content::GlobalFrameRoutingId(
                  render_frame_host->GetProcess()->GetID(),
                  render_frame_host->GetRoutingID()),
            url.spec(), main_frame));
  if (!main_frame)
    return;
  // Non-scriptlet cosmetic filters are only applied on the top-level frame
  if (web_contents()->GetMainFrame()) {
    web_contents()->GetMainFrame()->ExecuteJavaScriptInIsolatedWorld(
        base::UTF8ToUTF16(*BraveCosmeticResourcesTabHelper::observing_script_),
        base::NullCallback(), ISOLATED_WORLD_ID_CHROME_INTERNAL);
  }
}

void BraveCosmeticResourcesTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  ProcessURL(web_contents()->GetMainFrame(),
      web_contents()->GetLastCommittedURL(), true);
}

void BraveCosmeticResourcesTabHelper::ResourceLoadComplete(
    content::RenderFrameHost* render_frame_host,
    const content::GlobalRequestID& request_id,
    const blink::mojom::ResourceLoadInfo& resource_load_info) {
  ProcessURL(render_frame_host, resource_load_info.final_url, false);
}

void BraveCosmeticResourcesTabHelper::HiddenClassIdSelectors(
    content::RenderFrameHost* render_frame_host,
    const std::vector<std::string>& classes,
    const std::vector<std::string>& ids) {
  g_brave_browser_process->ad_block_service()->GetTaskRunner()->
      PostTaskAndReplyWithResult(FROM_HERE,
          base::BindOnce(&GetHiddenClassIdSelectorsOnTaskRunner, classes, ids,
              exceptions_),
          base::BindOnce(&BraveCosmeticResourcesTabHelper::
              GetHiddenClassIdSelectorsOnUI, AsWeakPtr(),
              content::GlobalFrameRoutingId(
                  render_frame_host->GetProcess()->GetID(),
                  render_frame_host->GetRoutingID())));
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveCosmeticResourcesTabHelper)
