/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/cosmetic_filters/renderer/cosmetic_filters_js_handler.h"

#include <optional>
#include <utility>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/metrics/histogram_macros.h"
#include "base/no_destructor.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/trace_event.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/content_settings/renderer/brave_content_settings_agent_impl.h"
#include "brave/components/cosmetic_filters/resources/grit/cosmetic_filters_generated_map.h"
#include "components/content_settings/renderer/content_settings_agent_impl.h"
#include "content/public/renderer/render_frame.h"
#include "gin/arguments.h"
#include "gin/function_template.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_css_origin.h"
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
          let text = '(function() {\nconst scriptletGlobals = (() => {\nconst forwardedMapMethods = ["has", "get", "set"];\nconst handler = {\nget(target, prop) { if (forwardedMapMethods.includes(prop)) { return Map.prototype[prop].bind(target) } return target.get(prop); },\nset(target, prop, value) { if (!forwardedMapMethods.includes(prop)) { target.set(prop, value); } }\n};\nreturn new Proxy(new Map(%s), handler);\n})();\nlet deAmpEnabled = %s;\n' + %s + '})()';
          let script;
          try {
            script = document.createElement('script');
            const textNode = document.createTextNode(text);
            script.appendChild(textNode);
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
    R"({
        const CC = window.content_cosmetic
        if (CC.hide1pContent === undefined)
          CC.hide1pContent = %s;
        if (CC.generichide === undefined)
          CC.generichide = %s;
        if (CC.firstSelectorsPollingDelayMs === undefined)
          CC.firstSelectorsPollingDelayMs = %s;
        if (CC.switchToSelectorsPollingThreshold === undefined)
          CC.switchToSelectorsPollingThreshold = %s;
        if (CC.fetchNewClassIdRulesThrottlingMs === undefined)
          CC.fetchNewClassIdRulesThrottlingMs = %s;
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
              try {
                window.content_cosmetic.cosmeticStyleSheet.insertRule(
                  `${rule}`, nextIndex);
                if (!window.content_cosmetic.hide1pContent) {
                  window.content_cosmetic.allSelectorsToRules.set(
                    selector, nextIndex);
                  window.content_cosmetic.firstRunQueue.add(selector);
                }
                nextIndex++;
              } catch (e) {
                console.warn('Brave Shields ignored an invalid CSS injection: ' + rule)
              }
            }
          });
          if (!document.adoptedStyleSheets.includes(
              window.content_cosmetic.cosmeticStyleSheet)) {
            document.adoptedStyleSheets =
              [window.content_cosmetic.cosmeticStyleSheet,
                ...document.adoptedStyleSheets];
          };
        })();)";

const char kRemovalsInjectScript[] =
    R"((function() {
          const CC = window.content_cosmetic;
          CC.selectorsToRemove = %s;
          const dictToMap = (d) => d === undefined
            ? d
            : new Map(Object.entries(d));
          CC.classesToRemoveBySelector = dictToMap(%s);
          CC.attributesToRemoveBySelector = dictToMap(%s);
          CC.hasRemovals = (
            CC.selectorsToRemove !== undefined
            || CC.classesToRemoveBySelector !== undefined
            || CC.attributesToRemoveBySelector !== undefined
          );
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

// ID is used in TRACE_ID_WITH_SCOPE(). Must be unique accoss the process.
int MakeUniquePerfId() {
  static int counter = 0;
  ++counter;
  return counter;
}

constexpr const char TRACE_CATEGORY[] = "brave.adblock";

}  // namespace

namespace cosmetic_filters {

// A class to record performance events from content_filter.ts.
// The events are reported as async traces and/or UMAs and can be retrived by
// brave://tracing & brave://histograms.
class CosmeticFilterPerfTracker {
 public:
  int OnHandleMutationsBegin() {
    const auto event_id = MakeUniquePerfId();
    TRACE_EVENT_NESTABLE_ASYNC_BEGIN0(
        TRACE_CATEGORY, "HandleMutations",
        TRACE_ID_WITH_SCOPE("HandleMutations", event_id));
    return event_id;
  }

  void OnHandleMutationsEnd(int event_id) {
    TRACE_EVENT_NESTABLE_ASYNC_END0(
        TRACE_CATEGORY, "HandleMutations",
        TRACE_ID_WITH_SCOPE("HandleMutations", event_id));
  }

  int OnQuerySelectorsBegin() {
    const auto event_id = MakeUniquePerfId();
    TRACE_EVENT_NESTABLE_ASYNC_BEGIN0(
        TRACE_CATEGORY, "QuerySelectors",
        TRACE_ID_WITH_SCOPE("QuerySelectors", event_id));
    return event_id;
  }

  void OnQuerySelectorsEnd(int event_id) {
    TRACE_EVENT_NESTABLE_ASYNC_END0(
        TRACE_CATEGORY, "QuerySelectors",
        TRACE_ID_WITH_SCOPE("QuerySelectors", event_id));
  }
};

CosmeticFiltersJSHandler::CosmeticFiltersJSHandler(
    content::RenderFrame* render_frame,
    const int32_t isolated_world_id)
    : render_frame_(render_frame),
      isolated_world_id_(isolated_world_id),
      enabled_1st_party_cf_(false) {
  EnsureConnected();

  const bool perf_tracker_enabled = base::FeatureList::IsEnabled(
      brave_shields::features::kCosmeticFilteringExtraPerfMetrics);
  if (perf_tracker_enabled) {
    perf_tracker_ = std::make_unique<CosmeticFilterPerfTracker>();
  }
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
  const auto url = url_.Resolve(url_string);
  if (!url.is_valid())
    return false;

  return net::registry_controlled_domains::SameDomainOrHost(
      url, url_, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

void CosmeticFiltersJSHandler::AddJavaScriptObjectToFrame(
    v8::Local<v8::Context> context) {
  CHECK(render_frame_);
  v8::Isolate* isolate =
      render_frame_->GetWebFrame()->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope(isolate);
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);

  CreateWorkerObject(isolate, context);
  bundle_injected_ = false;
}

// Stylesheets injected this way will be able to override `!important` styles
// from in-page styles, but cannot be reverted.
// `WebDocument::RemoveInsertedStyleSheet` works, but using a single stylesheet
// per rule has a significant performance impact and should be avoided.
void CosmeticFiltersJSHandler::InjectStylesheet(const std::string& stylesheet) {
  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();

  blink::WebStyleSheetKey* style_sheet_key = nullptr;
  blink::WebString stylesheet_webstring =
      blink::WebString::FromUTF8(stylesheet);
  web_frame->GetDocument().InsertStyleSheet(
      stylesheet_webstring, style_sheet_key, blink::WebCssOrigin::kUser);
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

  if (perf_tracker_) {
    BindFunctionToObject(
        isolate, javascript_object, "onHandleMutationsBegin",
        base::BindRepeating(&CosmeticFilterPerfTracker::OnHandleMutationsBegin,
                            base::Unretained(perf_tracker_.get())));
    BindFunctionToObject(
        isolate, javascript_object, "onHandleMutationsEnd",
        base::BindRepeating(&CosmeticFilterPerfTracker::OnHandleMutationsEnd,
                            base::Unretained(perf_tracker_.get())));
    BindFunctionToObject(
        isolate, javascript_object, "onQuerySelectorsBegin",
        base::BindRepeating(&CosmeticFilterPerfTracker::OnQuerySelectorsBegin,
                            base::Unretained(perf_tracker_.get())));
    BindFunctionToObject(
        isolate, javascript_object, "onQuerySelectorsEnd",
        base::BindRepeating(&CosmeticFilterPerfTracker::OnQuerySelectorsEnd,
                            base::Unretained(perf_tracker_.get())));
  }
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
    render_frame_->GetBrowserInterfaceBroker().GetInterface(
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
    std::optional<base::OnceClosure> callback) {
  resources_dict_ = std::nullopt;
  url_ = url;
  enabled_1st_party_cf_ = false;

  // Trivially, don't make exceptions for malformed URLs.
  if (!EnsureConnected() || url_.is_empty() || !url_.is_valid())
    return false;

  auto* content_settings =
      static_cast<content_settings::BraveContentSettingsAgentImpl*>(
          content_settings::ContentSettingsAgentImpl::Get(render_frame_));

  const bool force_cosmetic_filtering =
      render_frame_->GetBlinkPreferences().force_cosmetic_filtering;
  if (!force_cosmetic_filtering &&
      !content_settings->IsCosmeticFilteringEnabled(url_)) {
    return false;
  }

  enabled_1st_party_cf_ =
      force_cosmetic_filtering ||
      render_frame_->GetWebFrame()->IsCrossOriginToOutermostMainFrame() ||
      content_settings->IsFirstPartyCosmeticFilteringEnabled(url_) ||
      net::registry_controlled_domains::SameDomainOrHost(
          url_,
          url::Origin::CreateFromNormalizedTuple("https", "youtube.com", 443),
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  if (callback.has_value()) {
    SCOPED_UMA_HISTOGRAM_TIMER_MICROS(
        "Brave.CosmeticFilters.UrlCosmeticResources");
    TRACE_EVENT1("brave.adblock", "UrlCosmeticResources", "url", url_.spec());
    cosmetic_filters_resources_->UrlCosmeticResources(
        url_.spec(), enabled_1st_party_cf_,
        base::BindOnce(&CosmeticFiltersJSHandler::OnUrlCosmeticResources,
                       base::Unretained(this), std::move(callback.value())));
  } else {
    TRACE_EVENT1("brave.adblock", "UrlCosmeticResourcesSync", "url",
                 url_.spec());
    SCOPED_UMA_HISTOGRAM_TIMER_MICROS(
        "Brave.CosmeticFilters.UrlCosmeticResourcesSync");
    base::Value result;
    cosmetic_filters_resources_->UrlCosmeticResources(
        url_.spec(), enabled_1st_party_cf_, &result);

    auto* dict = result.GetIfDict();
    if (dict)
      resources_dict_ = std::move(*dict);
  }

  return true;
}

void CosmeticFiltersJSHandler::OnUrlCosmeticResources(
    base::OnceClosure callback,
    base::Value result) {
  if (!EnsureConnected())
    return;

  auto* dict = result.GetIfDict();
  if (dict)
    resources_dict_ = std::move(*dict);

  std::move(callback).Run();
}

void CosmeticFiltersJSHandler::ApplyRules(bool de_amp_enabled) {
  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
  if (!resources_dict_ || web_frame->IsProvisional())
    return;

  SCOPED_UMA_HISTOGRAM_TIMER_MICROS("Brave.CosmeticFilters.ApplyRules");
  TRACE_EVENT1("brave.adblock", "ApplyRules", "url", url_.spec());

  std::string scriptlet_script;
  base::Value* injected_script = resources_dict_->Find("injected_script");

  if (injected_script &&
      base::JSONWriter::Write(*injected_script, &scriptlet_script)) {
    const bool scriptlet_debug_enabled = base::FeatureList::IsEnabled(
        brave_shields::features::kBraveAdblockScriptletDebugLogs);

    scriptlet_script = base::StringPrintf(
        kScriptletInitScript,
        scriptlet_debug_enabled ? "[[\"canDebug\", true]]" : "",
        de_amp_enabled ? "true" : "false", scriptlet_script.c_str());
  }
  if (!scriptlet_script.empty()) {
    web_frame->ExecuteScriptInIsolatedWorld(
        isolated_world_id_,
        blink::WebScriptSource(blink::WebString::FromUTF8(scriptlet_script)),
        blink::BackForwardCacheAware::kAllow);
  }

  // Working on css rules
  generichide_ = resources_dict_->FindBool("generichide").value_or(false);
  namespace bf = brave_shields::features;
  std::string cosmetic_filtering_init_script = base::StringPrintf(
      kCosmeticFilteringInitScript, enabled_1st_party_cf_ ? "true" : "false",
      generichide_ ? "true" : "false",
      render_frame_->IsMainFrame()
          ? "undefined"
          : bf::kCosmeticFilteringSubFrameFirstSelectorsPollingDelayMs.Get()
                .c_str(),
      bf::kCosmeticFilteringswitchToSelectorsPollingThreshold.Get().c_str(),
      bf::kCosmeticFilteringFetchNewClassIdRulesThrottlingMs.Get().c_str());
  std::string pre_init_script = base::StringPrintf(
      kPreInitScript, cosmetic_filtering_init_script.c_str());

  web_frame->ExecuteScriptInIsolatedWorld(
      isolated_world_id_,
      blink::WebScriptSource(blink::WebString::FromUTF8(pre_init_script)),
      blink::BackForwardCacheAware::kAllow);
  ExecuteObservingBundleEntryPoint();

  CSSRulesRoutine(*resources_dict_);

  bool has_removals = false;
  //: remove()
  std::string remove_selectors_json;
  const auto* remove_selectors_list =
      resources_dict_->FindList("remove_selectors");
  if (remove_selectors_list && !remove_selectors_list->empty()) {
    base::JSONWriter::Write(*remove_selectors_list, &remove_selectors_json);
    has_removals = true;
  } else {
    remove_selectors_json = "undefined";
  }

  //: remove_classes
  std::string remove_classes_json;
  const auto* remove_classes_dictionary =
      resources_dict_->FindDict("remove_classes");
  if (remove_classes_dictionary && !remove_classes_dictionary->empty()) {
    base::JSONWriter::Write(*remove_classes_dictionary, &remove_classes_json);
    has_removals = true;
  }

  //: remove_attrs
  std::string remove_attrs_json;
  const auto* remove_attrs_dictionary =
      resources_dict_->FindDict("remove_attrs");
  if (remove_attrs_dictionary && !remove_attrs_dictionary->empty()) {
    base::JSONWriter::Write(*remove_attrs_dictionary, &remove_attrs_json);
    has_removals = true;
  }

  if (has_removals) {
    // Building a script for removals
    std::string new_selectors_script = base::StringPrintf(
        kRemovalsInjectScript, remove_selectors_json.c_str(),
        remove_classes_json.c_str(), remove_attrs_json.c_str());
    web_frame->ExecuteScriptInIsolatedWorld(
        isolated_world_id_,
        blink::WebScriptSource(
            blink::WebString::FromUTF8(new_selectors_script)),
        blink::BackForwardCacheAware::kAllow);
  }
}

void CosmeticFiltersJSHandler::CSSRulesRoutine(
    const base::Value::Dict& resources_dict) {
  SCOPED_UMA_HISTOGRAM_TIMER_MICROS("Brave.CosmeticFilters.CSSRulesRoutine");
  TRACE_EVENT1("brave.adblock", "CSSRulesRoutine", "url", url_.spec());

  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
  const auto* cf_exceptions_list = resources_dict.FindList("exceptions");
  if (cf_exceptions_list) {
    for (const auto& item : *cf_exceptions_list) {
      DCHECK(item.is_string());
      exceptions_.push_back(item.GetString());
    }
  }
  // If its a vetted engine AND we're not in aggressive mode, don't apply
  // cosmetic filtering from the default engine.
  const auto* hide_selectors_list =
      (IsVettedSearchEngine(url_) && !enabled_1st_party_cf_)
          ? nullptr
          : resources_dict.FindList("hide_selectors");

  std::string stylesheet = "";

  if (hide_selectors_list && !hide_selectors_list->empty()) {
    // treat `hide_selectors` the same as `force_hide_selectors` if aggressive
    // mode is enabled.
    if (enabled_1st_party_cf_) {
      for (auto& selector : *hide_selectors_list) {
        DCHECK(selector.is_string());
        stylesheet += selector.GetString() + "{display:none !important}";
      }
    } else {
      std::string json_selectors;
      base::JSONWriter::Write(*hide_selectors_list, &json_selectors);
      if (json_selectors.empty()) {
        json_selectors = "[]";
      }
      // Building a script for stylesheet modifications
      std::string new_selectors_script = base::StringPrintf(
          kHideSelectorsInjectScript, json_selectors.c_str());
      web_frame->ExecuteScriptInIsolatedWorld(
          isolated_world_id_,
          blink::WebScriptSource(
              blink::WebString::FromUTF8(new_selectors_script)),
          blink::BackForwardCacheAware::kAllow);
    }
  }

  const auto* force_hide_selectors_list =
      resources_dict.FindList("force_hide_selectors");
  if (force_hide_selectors_list) {
    for (auto& selector : *force_hide_selectors_list) {
      DCHECK(selector.is_string());
      stylesheet += selector.GetString() + "{display:none !important}";
    }
  }

  const auto* style_selectors_dictionary =
      resources_dict.FindDict("style_selectors");
  if (style_selectors_dictionary) {
    for (const auto kv : *style_selectors_dictionary) {
      DCHECK(kv.second.is_list());
      std::string selector = kv.first;
      const auto& styles = kv.second.GetList();
      stylesheet += selector + '{';
      for (auto& style : styles) {
        DCHECK(style.is_string());
        stylesheet += style.GetString() + ';';
      }
      stylesheet += '}';
    }
  }

  if (!stylesheet.empty()) {
    InjectStylesheet(stylesheet);
  }

  if (!enabled_1st_party_cf_)
    ExecuteObservingBundleEntryPoint();
}

void CosmeticFiltersJSHandler::OnHiddenClassIdSelectors(
    base::Value::Dict result) {
  if (generichide_) {
    return;
  }

  SCOPED_UMA_HISTOGRAM_TIMER_MICROS(
      "Brave.CosmeticFilters.OnHiddenClassIdSelectors");
  TRACE_EVENT1("brave.adblock", "OnHiddenClassIdSelectors", "url", url_.spec());

  base::Value::List* hide_selectors = result.FindList("hide_selectors");
  DCHECK(hide_selectors);

  base::Value::List* force_hide_selectors =
      result.FindList("force_hide_selectors");
  DCHECK(force_hide_selectors);

  if (force_hide_selectors->size() != 0) {
    std::string stylesheet = "";
    for (auto& selector : *force_hide_selectors) {
      DCHECK(selector.is_string());
      stylesheet += selector.GetString() + "{display:none !important}";
    }
    InjectStylesheet(stylesheet);
  }

  // If its a vetted engine AND we're not in aggressive
  // mode, don't check elements from the default engine (in hide_selectors).
  if (!enabled_1st_party_cf_ && IsVettedSearchEngine(url_))
    return;

  if (enabled_1st_party_cf_) {
    std::string stylesheet = "";
    for (auto& selector : *hide_selectors) {
      DCHECK(selector.is_string());
      stylesheet += selector.GetString() + "{display:none !important}";
    }
    InjectStylesheet(stylesheet);
  } else {
    blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
    std::string json_selectors;
    if (!base::JSONWriter::Write(*hide_selectors, &json_selectors) ||
        json_selectors.empty()) {
      json_selectors = "[]";
    }
    // Building a script for stylesheet modifications
    std::string new_selectors_script =
        base::StringPrintf(kHideSelectorsInjectScript, json_selectors.c_str());
    if (hide_selectors->size() != 0) {
      web_frame->ExecuteScriptInIsolatedWorld(
          isolated_world_id_,
          blink::WebScriptSource(
              blink::WebString::FromUTF8(new_selectors_script)),
          blink::BackForwardCacheAware::kAllow);
    }

    if (!enabled_1st_party_cf_)
      ExecuteObservingBundleEntryPoint();
  }
}

void CosmeticFiltersJSHandler::ExecuteObservingBundleEntryPoint() {
  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
  DCHECK(web_frame);

  if (!bundle_injected_) {
    SCOPED_UMA_HISTOGRAM_TIMER_MICROS(
        "Brave.CosmeticFilters.ExecuteObservingBundleEntryPoint");
    TRACE_EVENT1("brave.adblock", "ExecuteObservingBundleEntryPoint", "url",
                 url_.spec());

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
