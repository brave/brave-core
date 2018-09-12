/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_EXTENSIONS_RENDERER_BRAVE_SHIELDS_CONTENT_SETTING_H_
#define BRAVE_EXTENSIONS_RENDERER_BRAVE_SHIELDS_CONTENT_SETTING_H_

#include <string>

#include "base/macros.h"
#include "extensions/renderer/bindings/argument_spec.h"
#include "gin/wrappable.h"
#include "v8/include/v8.h"

namespace base {
class DictionaryValue;
class ListValue;
}

namespace gin {
class Arguments;
}

namespace extensions {
class APIEventHandler;
class APIRequestHandler;
class BindingAccessChecker;

// The custom implementation of the contentSettings.ContentSetting type exposed
// to APIs.
class BraveShieldsContentSetting final
    : public gin::Wrappable<BraveShieldsContentSetting> {
 public:
  ~BraveShieldsContentSetting() override;

  // Creates a ContentSetting object for the given property.
  static v8::Local<v8::Object> Create(
      v8::Isolate* isolate,
      const std::string& property_name,
      const base::ListValue* property_values,
      APIRequestHandler* request_handler,
      APIEventHandler* event_handler,
      APITypeReferenceMap* type_refs,
      const BindingAccessChecker* access_checker);

  static gin::WrapperInfo kWrapperInfo;

  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

 private:
  BraveShieldsContentSetting(APIRequestHandler* request_handler,
                 const APITypeReferenceMap* type_refs,
                 const BindingAccessChecker* access_checker,
                 const std::string& pref_name,
                 const base::DictionaryValue& argument_spec);

  // JS function handlers:
  void Get(gin::Arguments* arguments);
  void Set(gin::Arguments* arguments);

  // Common function handling endpoint.
  void HandleFunction(const std::string& function_name,
                      gin::Arguments* arguments);

  APIRequestHandler* request_handler_;

  const APITypeReferenceMap* type_refs_;

  const BindingAccessChecker* const access_checker_;

  // The name of the preference this ContentSetting is managing.
  std::string pref_name_;

  // The type of argument that calling set() on the ContentSetting expects
  // (since different settings can take a different type of argument depending
  // on the preference it manages).
  ArgumentSpec argument_spec_;

  DISALLOW_COPY_AND_ASSIGN(BraveShieldsContentSetting);
};

}  // namespace extensions

#endif  // BRAVE_EXTENSIONS_RENDERER_BRAVE_SHIELDS_CONTENT_SETTING_H_
