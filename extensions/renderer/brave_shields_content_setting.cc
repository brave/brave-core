/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/extensions/renderer/brave_shields_content_setting.h"

#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "content/public/common/console_message_level.h"
#include "extensions/renderer/bindings/api_binding_types.h"
#include "extensions/renderer/bindings/api_binding_util.h"
#include "extensions/renderer/bindings/api_invocation_errors.h"
#include "extensions/renderer/bindings/api_request_handler.h"
#include "extensions/renderer/bindings/api_signature.h"
#include "extensions/renderer/bindings/api_type_reference_map.h"
#include "extensions/renderer/bindings/binding_access_checker.h"
#include "extensions/renderer/bindings/js_runner.h"
#include "extensions/renderer/console.h"
#include "extensions/renderer/script_context_set.h"
#include "gin/arguments.h"
#include "gin/converter.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"

namespace extensions {

v8::Local<v8::Object> BraveShieldsContentSetting::Create(
    v8::Isolate* isolate,
    const std::string& property_name,
    const base::ListValue* property_values,
    APIRequestHandler* request_handler,
    APIEventHandler* event_handler,
    APITypeReferenceMap* type_refs,
    const BindingAccessChecker* access_checker) {
  std::string pref_name;
  CHECK(property_values->GetString(0u, &pref_name));
  const base::DictionaryValue* value_spec = nullptr;
  CHECK(property_values->GetDictionary(1u, &value_spec));

  gin::Handle<BraveShieldsContentSetting> handle = gin::CreateHandle(
      isolate, new BraveShieldsContentSetting(request_handler, type_refs, access_checker,
                                  pref_name, *value_spec));
  return handle.ToV8().As<v8::Object>();
}

BraveShieldsContentSetting::BraveShieldsContentSetting(APIRequestHandler* request_handler,
                               const APITypeReferenceMap* type_refs,
                               const BindingAccessChecker* access_checker,
                               const std::string& pref_name,
                               const base::DictionaryValue& set_value_spec)
    : request_handler_(request_handler),
      type_refs_(type_refs),
      access_checker_(access_checker),
      pref_name_(pref_name),
      argument_spec_(ArgumentType::OBJECT) {
  // The set() call takes an object { setting: { type: <t> }, ... }, where <t>
  // is the custom set() argument specified above by value_spec.
  ArgumentSpec::PropertiesMap properties;
  properties["primaryPattern"] =
      std::make_unique<ArgumentSpec>(ArgumentType::STRING);
  {
    auto secondary_pattern_spec =
        std::make_unique<ArgumentSpec>(ArgumentType::STRING);
    secondary_pattern_spec->set_optional(true);
    properties["secondaryPattern"] = std::move(secondary_pattern_spec);
  }
  {
    auto resource_identifier_spec =
        std::make_unique<ArgumentSpec>(ArgumentType::REF);
    resource_identifier_spec->set_ref("braveShields.ResourceIdentifier");
    resource_identifier_spec->set_optional(true);
    properties["resourceIdentifier"] = std::move(resource_identifier_spec);
  }
  {
    auto scope_spec = std::make_unique<ArgumentSpec>(ArgumentType::REF);
    scope_spec->set_ref("braveShields.Scope");
    scope_spec->set_optional(true);
    properties["scope"] = std::move(scope_spec);
  }
  properties["setting"] = std::make_unique<ArgumentSpec>(set_value_spec);
  argument_spec_.set_properties(std::move(properties));
}

BraveShieldsContentSetting::~BraveShieldsContentSetting() = default;

gin::WrapperInfo BraveShieldsContentSetting::kWrapperInfo = {gin::kEmbedderNativeGin};

gin::ObjectTemplateBuilder BraveShieldsContentSetting::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return Wrappable<BraveShieldsContentSetting>::GetObjectTemplateBuilder(isolate)
      .SetMethod("get", &BraveShieldsContentSetting::Get)
      .SetMethod("set", &BraveShieldsContentSetting::Set);
}

const char* BraveShieldsContentSetting::GetTypeName() {
  return "ContentSetting";
}

void BraveShieldsContentSetting::Get(gin::Arguments* arguments) {
  HandleFunction("get", arguments);
}

void BraveShieldsContentSetting::Set(gin::Arguments* arguments) {
  HandleFunction("set", arguments);
}

void BraveShieldsContentSetting::HandleFunction(const std::string& method_name,
                                    gin::Arguments* arguments) {
  v8::Isolate* isolate = arguments->isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = arguments->GetHolderCreationContext();

  if (!binding::IsContextValidOrThrowError(context))
    return;

  std::vector<v8::Local<v8::Value>> argument_list = arguments->GetAll();

  std::string full_name = "braveShields.ContentSetting." + method_name;

  if (!access_checker_->HasAccessOrThrowError(context, full_name))
    return;

  std::unique_ptr<base::ListValue> converted_arguments;
  v8::Local<v8::Function> callback;
  std::string error;
  const APISignature* signature = type_refs_->GetTypeMethodSignature(full_name);
  if (!signature->ParseArgumentsToJSON(context, argument_list, *type_refs_,
                                       &converted_arguments, &callback,
                                       &error)) {
    arguments->ThrowTypeError(api_errors::InvocationError(
        full_name, signature->GetExpectedSignature(), error));
    return;
  }

  if (method_name == "set") {
    v8::Local<v8::Value> value = argument_list[0];
    // The set schema included in the Schema object is generic, since it varies
    // per-setting. However, this is only ever for a single setting, so we can
    // enforce the types more thoroughly.
    // Note: we do this *after* checking if the setting is deprecated, since
    // this validation will fail for deprecated settings.
    std::string error;
    if (!value.IsEmpty() &&
        !argument_spec_.ParseArgument(context, value, *type_refs_, nullptr,
                                      nullptr, &error)) {
      arguments->ThrowTypeError("Invalid invocation: " + error);
      return;
    }
  }

  converted_arguments->Insert(0u, std::make_unique<base::Value>(pref_name_));
  request_handler_->StartRequest(
      context, "braveShields." + method_name, std::move(converted_arguments),
      callback, v8::Local<v8::Function>(), binding::RequestThread::UI);
}

}  // namespace extensions
