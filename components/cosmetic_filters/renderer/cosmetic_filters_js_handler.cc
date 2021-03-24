/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/cosmetic_filters/renderer/cosmetic_filters_js_handler.h"

#include "base/bind.h"
#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/cosmetic_filters/resources/grit/cosmetic_filters_generated_map.h"
#include "content/public/renderer/render_frame.h"
#include "gin/arguments.h"
#include "gin/function_template.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "ui/base/resource/resource_bundle.h"
#include "v8/include/v8.h"

namespace {

static base::NoDestructor<std::string> g_observing_script("");

static base::NoDestructor<std::vector<std::string>> g_vetted_search_engines(
    {"duckduckgo", "qwant", "bing", "startpage", "google", "yandex", "ecosia"});

const char kScriptletInitScript[] =
    R"((function() {
          let text = %s;
          let script;
          try {
            script = document.createElement('script');
            const textNode = document.createTextNode(text);
            script.appendChild(textNode);;
            (document.head || document.documentElement).appendChild(script);
          } catch (ex) {
            /* Unused catch */
          }
          if (script) {
            if (script.parentNode) {
              script.parentNode.removeChild(script);
            }
            script.textContent = '';
          }
        })();)";

const char kPreInitScript[] =
    R"((function() {
          if (window.content_cosmetic == undefined) {
            window.content_cosmetic = {};
          }
          %s
        })();)";

const char kCosmeticFilteringInitScript[] =
    R"(if (window.content_cosmetic.hide1pContent === undefined) {
        window.content_cosmetic.hide1pContent = %s;
       }
       if (window.content_cosmetic.generichide === undefined) {
         window.content_cosmetic.generichide = %s;
       })";

const char kHideSelectorsInjectScript[] =
    R"((function() {
          let nextIndex =
              window.content_cosmetic.cosmeticStyleSheet.rules.length;
          const selectors = %s;
          selectors.forEach(selector => {
            if ((typeof selector === 'string') &&
                (window.content_cosmetic.hide1pContent ||
                !window.content_cosmetic.allSelectorsToRules.has(selector))) {
              let rule = selector + '{display:none !important;}';
              window.content_cosmetic.cosmeticStyleSheet.insertRule(
                `${rule}`, nextIndex);
              if (!window.content_cosmetic.hide1pContent) {
                window.content_cosmetic.allSelectorsToRules.set(
                  selector, nextIndex);
                window.content_cosmetic.firstRunQueue.add(selector);
              }
              nextIndex++;
            }
          });
          if (!document.adoptedStyleSheets.includes(
              window.content_cosmetic.cosmeticStyleSheet)) {
            document.adoptedStyleSheets =
              [window.content_cosmetic.cosmeticStyleSheet,
                ...document.adoptedStyleSheets];
          };
        })();)";

const char kForceHideSelectorsInjectScript[] =
    R"((function() {
          let nextIndex =
              window.content_cosmetic.cosmeticStyleSheet.rules.length;
          const selectors = %s;
          selectors.forEach(selector => {
            if (typeof selector === 'string') {
              let rule = selector + '{display:none !important;}';
              window.content_cosmetic.cosmeticStyleSheet.insertRule(
                `${rule}`, nextIndex);
              if (!window.content_cosmetic.hide1pContent) {
                window.content_cosmetic.allSelectorsToRules.set(
                  selector, nextIndex);
              }
              nextIndex++;
            }
          });
          if (!document.adoptedStyleSheets.includes(
              window.content_cosmetic.cosmeticStyleSheet)) {
            document.adoptedStyleSheets =
              [window.content_cosmetic.cosmeticStyleSheet,
                ...document.adoptedStyleSheets];
          };
        })();)";

const char kStyleSelectorsInjectScript[] =
    R"((function() {
          let nextIndex =
              window.content_cosmetic.cosmeticStyleSheet.rules.length;
          const selectors = %s;
          for (let selector in selectors) {
            if (window.content_cosmetic.hide1pContent ||
                !window.content_cosmetic.allSelectorsToRules.has(selector)) {
              let rule = selector + '{';
              selectors[selector].forEach(prop => {
                if (!rule.endsWith('{')) {
                  rule += ';';
                }
                rule += prop;
              });
              rule += '}';
              window.content_cosmetic.cosmeticStyleSheet.insertRule(
                `${rule}`, nextIndex);
              if (!window.content_cosmetic.hide1pContent) {
                window.content_cosmetic.allSelectorsToRules.set(
                  selector, nextIndex);
              }
              nextIndex++;
            };
          };
          if (!document.adoptedStyleSheets.includes(
                window.content_cosmetic.cosmeticStyleSheet)){
             document.adoptedStyleSheets =
               [window.content_cosmetic.cosmeticStyleSheet,
                 ...document.adoptedStyleSheets];
          };
        })();)";

std::string LoadDataResource(const int id) {
  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  if (resource_bundle.IsGzipped(id)) {
    return resource_bundle.LoadDataResourceString(id);
  }

  return resource_bundle.GetRawDataResource(id).as_string();
}

bool IsVettedSearchEngine(const GURL& url) {
  std::string domain_and_registry =
      net::registry_controlled_domains::GetDomainAndRegistry(
          url, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  size_t registry_len = net::registry_controlled_domains::GetRegistryLength(
      url, net::registry_controlled_domains::EXCLUDE_UNKNOWN_REGISTRIES,
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  if (domain_and_registry.length() > registry_len + 1) {
    std::string host = domain_and_registry.substr(
        0, domain_and_registry.length() - registry_len - 1);
    for (size_t i = 0; i < g_vetted_search_engines->size(); i++) {
      if ((*g_vetted_search_engines)[i] == host)
        return true;
    }
  }

  return false;
}

}  // namespace

namespace cosmetic_filters {

CosmeticFiltersJSHandler::CosmeticFiltersJSHandler(
    content::RenderFrame* render_frame,
    const int32_t isolated_world_id)
    : render_frame_(render_frame),
      isolated_world_id_(isolated_world_id),
      enabled_1st_party_cf_(false) {
  if (g_observing_script->empty()) {
    *g_observing_script = LoadDataResource(kCosmeticFiltersGenerated[0].id);
  }
  EnsureConnected();
}

CosmeticFiltersJSHandler::~CosmeticFiltersJSHandler() = default;

void CosmeticFiltersJSHandler::HiddenClassIdSelectors(
    const std::string& input) {
  if (!EnsureConnected())
    return;

  cosmetic_filters_resources_->HiddenClassIdSelectors(
      input, exceptions_,
      base::BindOnce(&CosmeticFiltersJSHandler::OnHiddenClassIdSelectors,
                     base::Unretained(this)));
}

void CosmeticFiltersJSHandler::AddJavaScriptObjectToFrame(
    v8::Local<v8::Context> context) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);

  CreateWorkerObject(isolate, context);
}

void CosmeticFiltersJSHandler::CreateWorkerObject(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context) {
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Object> cosmetic_filters_obj;
  v8::Local<v8::Value> cosmetic_filters_value;
  if (!global->Get(context, gin::StringToV8(isolate, "cf_worker"))
           .ToLocal(&cosmetic_filters_value) ||
      !cosmetic_filters_value->IsObject()) {
    cosmetic_filters_obj = v8::Object::New(isolate);
    global
        ->Set(context, gin::StringToSymbol(isolate, "cf_worker"),
              cosmetic_filters_obj)
        .Check();
    BindFunctionsToObject(isolate, cosmetic_filters_obj);
  }
}

void CosmeticFiltersJSHandler::BindFunctionsToObject(
    v8::Isolate* isolate,
    v8::Local<v8::Object> javascript_object) {
  BindFunctionToObject(
      isolate, javascript_object, "hiddenClassIdSelectors",
      base::BindRepeating(&CosmeticFiltersJSHandler::HiddenClassIdSelectors,
                          base::Unretained(this)));
}

template <typename Sig>
void CosmeticFiltersJSHandler::BindFunctionToObject(
    v8::Isolate* isolate,
    v8::Local<v8::Object> javascript_object,
    const std::string& name,
    const base::RepeatingCallback<Sig>& callback) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  // Get the isolate associated with this object.
  javascript_object
      ->Set(context, gin::StringToSymbol(isolate, name),
            gin::CreateFunctionTemplate(isolate, callback)
                ->GetFunction(context)
                .ToLocalChecked())
      .Check();
}

bool CosmeticFiltersJSHandler::EnsureConnected() {
  if (!cosmetic_filters_resources_.is_bound()) {
    render_frame_->GetBrowserInterfaceBroker()->GetInterface(
        cosmetic_filters_resources_.BindNewPipeAndPassReceiver());
  }

  return cosmetic_filters_resources_.is_bound();
}

void CosmeticFiltersJSHandler::ProcessURL(const GURL& url) {
  url_ = url;
  // Trivially, don't make exceptions for malformed URLs.
  if (!EnsureConnected() || url_.is_empty() || !url_.is_valid())
    return;

  cosmetic_filters_resources_->ShouldDoCosmeticFiltering(
      url_.spec(),
      base::BindOnce(&CosmeticFiltersJSHandler::OnShouldDoCosmeticFiltering,
                     base::Unretained(this)));
}

void CosmeticFiltersJSHandler::OnShouldDoCosmeticFiltering(
    bool enabled,
    bool first_party_enabled) {
  if (!enabled || !EnsureConnected())
    return;

  enabled_1st_party_cf_ = first_party_enabled;
  cosmetic_filters_resources_->UrlCosmeticResources(
      url_.spec(),
      base::BindOnce(&CosmeticFiltersJSHandler::OnUrlCosmeticResources,
                     base::Unretained(this)));
}

void CosmeticFiltersJSHandler::OnUrlCosmeticResources(base::Value result) {
  base::DictionaryValue* resources_dict;
  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
  if (!result.GetAsDictionary(&resources_dict) || web_frame->IsProvisional())
    return;

  std::string scriptlet_script;
  base::Value* injected_script = resources_dict->FindPath("injected_script");
  if (injected_script &&
      base::JSONWriter::Write(*injected_script, &scriptlet_script)) {
    scriptlet_script =
        base::StringPrintf(kScriptletInitScript, scriptlet_script.c_str());
  }
  if (!scriptlet_script.empty()) {
    web_frame->ExecuteScriptInIsolatedWorld(
        isolated_world_id_, blink::WebString::FromUTF8(scriptlet_script));
  }
  if (!render_frame_->IsMainFrame())
    return;

  // Working on css rules, we do that on a main frame only
  bool generichide = false;
  resources_dict->GetBoolean("generichide", &generichide);
  std::string cosmetic_filtering_init_script = base::StringPrintf(
      kCosmeticFilteringInitScript, enabled_1st_party_cf_ ? "true" : "false",
      generichide ? "true" : "false");
  std::string pre_init_script = base::StringPrintf(
      kPreInitScript, cosmetic_filtering_init_script.c_str());

  web_frame->ExecuteScriptInIsolatedWorld(
      isolated_world_id_, blink::WebString::FromUTF8(pre_init_script));
  web_frame->ExecuteScriptInIsolatedWorld(
      isolated_world_id_, blink::WebString::FromUTF8(*g_observing_script));

  CSSRulesRoutine(resources_dict);
}

void CosmeticFiltersJSHandler::CSSRulesRoutine(
    base::DictionaryValue* resources_dict) {
  // Otherwise, if its a vetted engine AND we're not in aggressive
  // mode, also don't do cosmetic filtering.
  if (!enabled_1st_party_cf_ && IsVettedSearchEngine(url_))
    return;

  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
  base::ListValue* cf_exceptions_list;
  if (resources_dict->GetList("exceptions", &cf_exceptions_list)) {
    for (size_t i = 0; i < cf_exceptions_list->GetSize(); i++) {
      exceptions_.push_back(cf_exceptions_list->GetList()[i].GetString());
    }
  }
  base::ListValue* hide_selectors_list;
  if (!resources_dict->GetList("hide_selectors", &hide_selectors_list)) {
    hide_selectors_list = nullptr;
  }
  base::ListValue* force_hide_selectors_list;
  if (!resources_dict->GetList("force_hide_selectors",
                               &force_hide_selectors_list)) {
    force_hide_selectors_list = nullptr;
  }

  if (hide_selectors_list && hide_selectors_list->GetSize() != 0) {
    std::string json_selectors;
    if (!base::JSONWriter::Write(*hide_selectors_list, &json_selectors) ||
        json_selectors.empty()) {
      json_selectors = "[]";
    }
    // Building a script for stylesheet modifications
    std::string new_selectors_script =
        base::StringPrintf(kHideSelectorsInjectScript, json_selectors.c_str());
    web_frame->ExecuteScriptInIsolatedWorld(
        isolated_world_id_, blink::WebString::FromUTF8(new_selectors_script));
  }

  if (force_hide_selectors_list && force_hide_selectors_list->GetSize() != 0) {
    std::string json_selectors;
    if (!base::JSONWriter::Write(*force_hide_selectors_list, &json_selectors) ||
        json_selectors.empty()) {
      json_selectors = "[]";
    }
    // Building a script for stylesheet modifications
    std::string new_selectors_script = base::StringPrintf(
        kForceHideSelectorsInjectScript, json_selectors.c_str());
    web_frame->ExecuteScriptInIsolatedWorld(
        isolated_world_id_, blink::WebString::FromUTF8(new_selectors_script));
  }

  base::DictionaryValue* style_selectors_dictionary = nullptr;
  if (resources_dict->GetDictionary("style_selectors",
                                    &style_selectors_dictionary)) {
    std::string json_selectors;
    if (!base::JSONWriter::Write(*style_selectors_dictionary,
                                 &json_selectors) ||
        json_selectors.empty()) {
      json_selectors = "[]";
    }
    std::string new_selectors_script =
        base::StringPrintf(kStyleSelectorsInjectScript, json_selectors.c_str());
    if (!json_selectors.empty()) {
      web_frame->ExecuteScriptInIsolatedWorld(
          isolated_world_id_, blink::WebString::FromUTF8(new_selectors_script));
    }
  }

  if (!enabled_1st_party_cf_) {
    web_frame->ExecuteScriptInIsolatedWorld(
        isolated_world_id_, blink::WebString::FromUTF8(*g_observing_script));
  }
}

void CosmeticFiltersJSHandler::OnHiddenClassIdSelectors(base::Value result) {
  // If its a vetted engine AND we're not in aggressive
  // mode, don't do cosmetic filtering.
  if (!enabled_1st_party_cf_ && IsVettedSearchEngine(url_))
    return;

  // We expect a List value from adblock service. That is
  // an extra check to be sure that adblock file exist and gives us
  // rules that we expect
  base::ListValue* selectors_list;
  if (!result.GetAsList(&selectors_list))
    return;

  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
  std::string json_selectors;
  if (!base::JSONWriter::Write(*selectors_list, &json_selectors) ||
      json_selectors.empty()) {
    json_selectors = "[]";
  }
  // Building a script for stylesheet modifications
  std::string new_selectors_script =
      base::StringPrintf(kHideSelectorsInjectScript, json_selectors.c_str());
  if (selectors_list->GetSize() != 0) {
    web_frame->ExecuteScriptInIsolatedWorld(
        isolated_world_id_, blink::WebString::FromUTF8(new_selectors_script));
  }

  if (!enabled_1st_party_cf_) {
    web_frame->ExecuteScriptInIsolatedWorld(
        isolated_world_id_, blink::WebString::FromUTF8(*g_observing_script));
  }
}

}  // namespace cosmetic_filters
