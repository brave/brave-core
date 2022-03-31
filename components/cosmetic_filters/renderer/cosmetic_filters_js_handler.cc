/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/cosmetic_filters/renderer/cosmetic_filters_js_handler.h"

#include <utility>

#include "base/bind.h"
#include "base/json/json_writer.h"
#include "base/metrics/histogram_macros.h"
#include "base/no_destructor.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/trace_event.h"
#include "brave/components/content_settings/renderer/brave_content_settings_agent_impl.h"
#include "brave/components/cosmetic_filters/resources/grit/cosmetic_filters_generated_map.h"
#include "components/content_settings/renderer/content_settings_agent_impl.h"
#include "content/public/renderer/render_frame.h"
#include "gin/arguments.h"
#include "gin/function_template.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "ui/base/resource/resource_bundle.h"
#include "v8/include/v8.h"

namespace {

static base::NoDestructor<std::vector<std::string>> g_vetted_search_engines(
    {"duckduckgo", "qwant", "bing", "startpage", "google", "yandex", "ecosia",
     "brave"});

// Entry point to content_cosmetic.ts script.
const char kObservingScriptletEntryPoint[] =
    "window.content_cosmetic.tryScheduleQueuePump()";

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
          const selectors = %s;
          selectors.forEach(selector => {
            if ((typeof selector === 'string') &&
                (window.content_cosmetic.hide1pContent ||
                !window.content_cosmetic.allSelectorsToRules.has(selector))) {
              let rule = selector + '{display:none !important;}';
              let ruleIndex = 0;
              if (!window.content_cosmetic.hide1pContent) {
                ruleIndex = window.content_cosmetic.nextRuleIndex;
                window.content_cosmetic.nextRuleIndex++;
                window.content_cosmetic.allSelectorsToRules.set(
                  selector, ruleIndex);
                window.content_cosmetic.firstRunQueue.add(selector);
              }
              window.cf_worker.injectStylesheet(rule, ruleIndex);
            }
          });
          window.content_cosmetic.scheduleQueuePump(false, false);
        })();)";

std::string LoadDataResource(const int id) {
  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  if (resource_bundle.IsGzipped(id)) {
    return resource_bundle.LoadDataResourceString(id);
  }

  return std::string(resource_bundle.GetRawDataResource(id));
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

bool CosmeticFiltersJSHandler::OnIsFirstParty(const std::string& url_string) {
  const auto url = GURL(url_string);
  if (!url.is_valid())
    return false;

  return net::registry_controlled_domains::SameDomainOrHost(
      url, url_, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

void CosmeticFiltersJSHandler::AddJavaScriptObjectToFrame(
    v8::Local<v8::Context> context) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);

  CreateWorkerObject(isolate, context);
  bundle_injected_ = false;
}

// If `id` is nonzero, then the same number can be later passed to
// `UninjectStylesheet` to remove the stylesheet from the page.
void CosmeticFiltersJSHandler::InjectStylesheet(const std::string& stylesheet,
                                                int id) {
  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();

  blink::WebStyleSheetKey* style_sheet_key = nullptr;
  if (id != 0) {
    // Prepend a Brave-specific string to avoid collisions with stylesheets
    // injected from other sources
    std::string key = "BraveCFRule" + std::to_string(id);
    inserted_stylesheet_ids.insert({id, std::make_unique<blink::WebString>(
                                            blink::WebString::FromASCII(key))});
    style_sheet_key = inserted_stylesheet_ids.at(id).get();
  }
  web_frame->GetDocument().InsertStyleSheet(
      blink::WebString::FromUTF8(stylesheet), style_sheet_key,
      blink::WebDocument::kUserOrigin);
}

void CosmeticFiltersJSHandler::UninjectStylesheet(int id) {
  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();

  DCHECK_NE(id, 0);

  auto i = inserted_stylesheet_ids.find(id);
  if (i != inserted_stylesheet_ids.end()) {
    std::unique_ptr<blink::WebStyleSheetKey> key = std::move(i->second);
    inserted_stylesheet_ids.erase(i);

    web_frame->GetDocument().RemoveInsertedStyleSheet(
        *key, blink::WebDocument::kUserOrigin);
  }
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
  BindFunctionToObject(
      isolate, javascript_object, "isFirstPartyUrl",
      base::BindRepeating(&CosmeticFiltersJSHandler::OnIsFirstParty,
                          base::Unretained(this)));
  BindFunctionToObject(
      isolate, javascript_object, "injectStylesheet",
      base::BindRepeating(&CosmeticFiltersJSHandler::InjectStylesheet,
                          base::Unretained(this)));
  BindFunctionToObject(
      isolate, javascript_object, "uninjectStylesheet",
      base::BindRepeating(&CosmeticFiltersJSHandler::UninjectStylesheet,
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
    cosmetic_filters_resources_.set_disconnect_handler(
        base::BindOnce(&CosmeticFiltersJSHandler::OnRemoteDisconnect,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  return cosmetic_filters_resources_.is_bound();
}

void CosmeticFiltersJSHandler::OnRemoteDisconnect() {
  cosmetic_filters_resources_.reset();
  EnsureConnected();
}

bool CosmeticFiltersJSHandler::ProcessURL(
    const GURL& url,
    absl::optional<base::OnceClosure> callback) {
  resources_dict_.reset();
  url_ = url;
  enabled_1st_party_cf_ = false;

  // Trivially, don't make exceptions for malformed URLs.
  if (!EnsureConnected() || url_.is_empty() || !url_.is_valid())
    return false;

  auto* content_settings =
      static_cast<content_settings::BraveContentSettingsAgentImpl*>(
          content_settings::ContentSettingsAgentImpl::Get(render_frame_));

  if (!content_settings->IsCosmeticFilteringEnabled(url_)) {
    return false;
  }

  enabled_1st_party_cf_ =
      content_settings->IsFirstPartyCosmeticFilteringEnabled(url_);

  if (callback.has_value()) {
    SCOPED_UMA_HISTOGRAM_TIMER_MICROS(
        "Brave.CosmeticFilters.UrlCosmeticResources");
    TRACE_EVENT1("brave.adblock", "UrlCosmeticResources", "url", url_.spec());
    cosmetic_filters_resources_->UrlCosmeticResources(
        url_.spec(),
        base::BindOnce(&CosmeticFiltersJSHandler::OnUrlCosmeticResources,
                       base::Unretained(this), std::move(callback.value())));
  } else {
    TRACE_EVENT1("brave.adblock", "UrlCosmeticResourcesSync", "url",
                 url_.spec());
    SCOPED_UMA_HISTOGRAM_TIMER_MICROS(
        "Brave.CosmeticFilters.UrlCosmeticResourcesSync");
    base::Value result;
    cosmetic_filters_resources_->UrlCosmeticResources(url_.spec(), &result);
    resources_dict_ = base::DictionaryValue::From(
        base::Value::ToUniquePtrValue(std::move(result)));
  }

  return true;
}

void CosmeticFiltersJSHandler::OnUrlCosmeticResources(
    base::OnceClosure callback,
    base::Value result) {
  if (!EnsureConnected())
    return;

  resources_dict_ = base::DictionaryValue::From(
      base::Value::ToUniquePtrValue(std::move(result)));
  std::move(callback).Run();
}

void CosmeticFiltersJSHandler::ApplyRules() {
  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
  if (!resources_dict_ || web_frame->IsProvisional())
    return;

  std::string scriptlet_script;
  base::Value* injected_script = resources_dict_->FindPath("injected_script");
  if (injected_script &&
      base::JSONWriter::Write(*injected_script, &scriptlet_script)) {
    scriptlet_script =
        base::StringPrintf(kScriptletInitScript, scriptlet_script.c_str());
  }
  if (!scriptlet_script.empty()) {
    web_frame->ExecuteScriptInIsolatedWorld(
        isolated_world_id_,
        blink::WebScriptSource(blink::WebString::FromUTF8(scriptlet_script)),
        blink::BackForwardCacheAware::kAllow);
  }

  if (!render_frame_->IsMainFrame())
    return;

  // Working on css rules, we do that on a main frame only
  generichide_ = resources_dict_->FindBoolKey("generichide").value_or(false);
  std::string cosmetic_filtering_init_script = base::StringPrintf(
      kCosmeticFilteringInitScript, enabled_1st_party_cf_ ? "true" : "false",
      generichide_ ? "true" : "false");
  std::string pre_init_script = base::StringPrintf(
      kPreInitScript, cosmetic_filtering_init_script.c_str());

  web_frame->ExecuteScriptInIsolatedWorld(
      isolated_world_id_,
      blink::WebScriptSource(blink::WebString::FromUTF8(pre_init_script)),
      blink::BackForwardCacheAware::kAllow);
  ExecuteObservingBundleEntryPoint();

  CSSRulesRoutine(resources_dict_.get());
}

void CosmeticFiltersJSHandler::CSSRulesRoutine(
    base::DictionaryValue* resources_dict) {
  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
  base::ListValue* cf_exceptions_list;
  if (resources_dict->GetList("exceptions", &cf_exceptions_list)) {
    for (size_t i = 0; i < cf_exceptions_list->GetList().size(); i++) {
      exceptions_.push_back(cf_exceptions_list->GetList()[i].GetString());
    }
  }
  // If its a vetted engine AND we're not in aggressive mode, don't apply
  // cosmetic filtering from the default engine.
  base::ListValue* hide_selectors_list;
  if (!resources_dict->GetList("hide_selectors", &hide_selectors_list) ||
      (IsVettedSearchEngine(url_) && !enabled_1st_party_cf_)) {
    hide_selectors_list = nullptr;
  }

  if (hide_selectors_list && hide_selectors_list->GetList().size() != 0) {
    std::string json_selectors;
    if (!base::JSONWriter::Write(*hide_selectors_list, &json_selectors) ||
        json_selectors.empty()) {
      json_selectors = "[]";
    }
    // Building a script for stylesheet modifications
    std::string new_selectors_script =
        base::StringPrintf(kHideSelectorsInjectScript, json_selectors.c_str());
    web_frame->ExecuteScriptInIsolatedWorld(
        isolated_world_id_,
        blink::WebScriptSource(
            blink::WebString::FromUTF8(new_selectors_script)),
        blink::BackForwardCacheAware::kAllow);
  }

  base::Value* force_hide_selectors_list =
      resources_dict->FindListKey("force_hide_selectors");
  if (force_hide_selectors_list &&
      force_hide_selectors_list->GetList().size() != 0) {
    std::string stylesheet = "";
    for (auto& selector : force_hide_selectors_list->GetList()) {
      DCHECK(selector.is_string());
      stylesheet += selector.GetString() + "{display:none !important}";
    }
    InjectStylesheet(stylesheet, 0);
  }

  base::Value* style_selectors_dictionary =
      resources_dict->FindDictKey("style_selectors");
  if (style_selectors_dictionary) {
    std::string stylesheet = "";
    for (const auto kv : style_selectors_dictionary->DictItems()) {
      std::string selector = kv.first;
      base::Value& styles = kv.second;
      DCHECK(styles.is_list());
      stylesheet += selector + '{';
      for (auto& style : styles.GetList()) {
        DCHECK(style.is_string());
        stylesheet += style.GetString() + ';';
      }
      stylesheet += '}';
    }
    InjectStylesheet(stylesheet, 0);
  }

  if (!enabled_1st_party_cf_)
    ExecuteObservingBundleEntryPoint();
}

void CosmeticFiltersJSHandler::OnHiddenClassIdSelectors(base::Value result) {
  if (generichide_) {
    return;
  }

  DCHECK(result.is_dict());

  base::Value* hide_selectors = result.FindListKey("hide_selectors");
  DCHECK(hide_selectors);

  base::Value* force_hide_selectors =
      result.FindListKey("force_hide_selectors");
  DCHECK(force_hide_selectors);

  if (force_hide_selectors->GetList().size() != 0) {
    std::string stylesheet = "";
    for (auto& selector : force_hide_selectors->GetList()) {
      DCHECK(selector.is_string());
      stylesheet += selector.GetString() + "{display:none !important}";
    }
    InjectStylesheet(stylesheet, 0);
  }

  // If its a vetted engine AND we're not in aggressive
  // mode, don't check elements from the default engine (in hide_selectors).
  if (!enabled_1st_party_cf_ && IsVettedSearchEngine(url_))
    return;

  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
  std::string json_selectors;
  if (!base::JSONWriter::Write(*hide_selectors, &json_selectors) ||
      json_selectors.empty()) {
    json_selectors = "[]";
  }
  // Building a script for stylesheet modifications
  std::string new_selectors_script =
      base::StringPrintf(kHideSelectorsInjectScript, json_selectors.c_str());
  if (hide_selectors->GetList().size() != 0) {
    web_frame->ExecuteScriptInIsolatedWorld(
        isolated_world_id_,
        blink::WebScriptSource(
            blink::WebString::FromUTF8(new_selectors_script)),
        blink::BackForwardCacheAware::kAllow);
  }

  if (!enabled_1st_party_cf_)
    ExecuteObservingBundleEntryPoint();
}

void CosmeticFiltersJSHandler::ExecuteObservingBundleEntryPoint() {
  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
  DCHECK(web_frame);

  if (!bundle_injected_) {
    static base::NoDestructor<std::string> s_observing_script(
        LoadDataResource(kCosmeticFiltersGenerated[0].id));
    bundle_injected_ = true;

    web_frame->ExecuteScriptInIsolatedWorld(
        isolated_world_id_,
        blink::WebScriptSource(blink::WebString::FromUTF8(*s_observing_script)),
        blink::BackForwardCacheAware::kAllow);

    // kObservingScriptletEntryPoint was called by `s_observing_script`.
    return;
  }

  web_frame->ExecuteScriptInIsolatedWorld(
      isolated_world_id_,
      blink::WebScriptSource(
          blink::WebString::FromUTF8(kObservingScriptletEntryPoint)),
      blink::BackForwardCacheAware::kAllow);
}

}  // namespace cosmetic_filters
