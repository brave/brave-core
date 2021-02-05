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

const char kPreInitScript[] =
    R"((function() {
          if (window.content_cosmetic == undefined) {
            window.content_cosmetic = {};
          }
          %s
          %s
        })();)";

const char kNonScriptletInitScript[] =
    R"(if (window.content_cosmetic.hide1pContent === undefined) {
        window.content_cosmetic.hide1pContent = %s;
       }
       if (window.content_cosmetic.generichide === undefined) {
         window.content_cosmetic.generichide = %s;
       })";

const char kSelectorsInjectScript[] =
    R"((function() {
          let nextIndex =
              window.content_cosmetic.cosmeticStyleSheet.rules.length;
          const selectors = %s;
          selectors.forEach(selector => {
            if ((typeof selector === 'string') &&
                !window.content_cosmetic.allSelectorsToRules.has(selector)) {
              let rule = selector + '{display:none !important;}';
              window.content_cosmetic.cosmeticStyleSheet.insertRule(
                `${rule}`, nextIndex);
              window.content_cosmetic.allSelectorsToRules.set(
                selector, nextIndex);
              nextIndex++;
              window.content_cosmetic.firstRunQueue.add(selector);
            }
          });
          if (!document.adoptedStyleSheets.includes(
              window.content_cosmetic.cosmeticStyleSheet)) {
            document.adoptedStyleSheets =
              [window.content_cosmetic.cosmeticStyleSheet];
          };
        })();)";

const char kStyleSelectorsInjectScript[] =
    R"((function() {
          let nextIndex =
              window.content_cosmetic.cosmeticStyleSheet.rules.length;
          const selectors = %s;
          for (let selector in selectors) {
            if (!window.content_cosmetic.allSelectorsToRules.has(selector)) {
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
              window.content_cosmetic.allSelectorsToRules.set(
                selector, nextIndex);
              nextIndex++;
            };
          };
          if (!document.adoptedStyleSheets.includes(
                window.content_cosmetic.cosmeticStyleSheet)){
             document.adoptedStyleSheets =
               [window.content_cosmetic.cosmeticStyleSheet];
          };
        })();)";

std::string LoadDataResource(const int id) {
  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  if (resource_bundle.IsGzipped(id)) {
    return resource_bundle.LoadDataResourceString(id);
  }

  return resource_bundle.GetRawDataResource(id).as_string();
}

bool IsVettedSearchEngine(const std::string& host) {
  for (size_t i = 0; i < g_vetted_search_engines->size(); i++) {
    size_t found_pos = host.find((*g_vetted_search_engines)[i]);
    if (found_pos != std::string::npos) {
      size_t last_dot_pos = host.find(".", found_pos + 1);
      if (last_dot_pos == std::string::npos)
        return false;
      if (host.find(".", last_dot_pos + 1) == std::string::npos &&
          (found_pos == 0 || host[found_pos - 1] == '/' ||
           host[found_pos - 1] == '\\' || host[found_pos - 1] == ':')) {
        return true;
      }
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
    *g_observing_script = LoadDataResource(kCosmeticFiltersGenerated[0].value);
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
  if (!EnsureConnected())
    return;

  url_ = url;
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
  if (!result.GetAsDictionary(&resources_dict)) {
    return;
  }

  std::string pre_init_script;
  std::string scriptlet_init_script;
  std::string non_scriptlet_init_script;
  std::string json_to_inject;
  base::Value* injected_script = resources_dict->FindPath("injected_script");
  if (injected_script &&
      base::JSONWriter::Write(*injected_script, &json_to_inject) &&
      json_to_inject.length() > 1) {
    scriptlet_init_script = json_to_inject;
  }
  if (render_frame_->IsMainFrame()) {
    bool generichide = false;
    resources_dict->GetBoolean("generichide", &generichide);
    non_scriptlet_init_script = base::StringPrintf(
        kNonScriptletInitScript, enabled_1st_party_cf_ ? "true" : "false",
        generichide ? "true" : "false");
  }
  pre_init_script =
      base::StringPrintf(kPreInitScript, scriptlet_init_script.c_str(),
                         non_scriptlet_init_script.c_str());
  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
  if (web_frame->IsProvisional())
    return;
  web_frame->ExecuteScriptInIsolatedWorld(
      isolated_world_id_, blink::WebString::FromUTF8(pre_init_script));
  if (!render_frame_->IsMainFrame())
    return;

  // Working on css rules, we do that on a main frame only
  web_frame->ExecuteScriptInIsolatedWorld(
      isolated_world_id_, blink::WebString::FromUTF8(*g_observing_script));

  CSSRulesRoutine(resources_dict);
}

void CosmeticFiltersJSHandler::CSSRulesRoutine(
    base::DictionaryValue* resources_dict) {
  // Trivially, don't make exceptions for malformed URLs.
  if (url_.is_empty() || !url_.is_valid())
    return;

  // Otherwise, if its a vetted engine AND we're not in aggressive
  // mode, also don't do cosmetic filtering.
  if (!enabled_1st_party_cf_ && IsVettedSearchEngine(url_.host()))
    return;

  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
  base::ListValue* cf_exceptions_list;
  if (resources_dict->GetList("exceptions", &cf_exceptions_list)) {
    for (size_t i = 0; i < cf_exceptions_list->GetSize(); i++) {
      exceptions_.push_back(cf_exceptions_list->GetList()[i].GetString());
    }
  }
  base::ListValue* hide_selectors_list;
  if (resources_dict->GetList("hide_selectors", &hide_selectors_list)) {
    std::string json_selectors;
    if (!base::JSONWriter::Write(*hide_selectors_list, &json_selectors) ||
        json_selectors.empty()) {
      json_selectors = "[]";
    }
    // Building a script for stylesheet modifications
    std::string new_selectors_script =
        base::StringPrintf(kSelectorsInjectScript, json_selectors.c_str());
    if (hide_selectors_list->GetSize() != 0) {
      web_frame->ExecuteScriptInIsolatedWorld(
          isolated_world_id_, blink::WebString::FromUTF8(new_selectors_script));
    }
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
  if (!enabled_1st_party_cf_ && IsVettedSearchEngine(url_.host()))
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
      base::StringPrintf(kSelectorsInjectScript, json_selectors.c_str());
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
