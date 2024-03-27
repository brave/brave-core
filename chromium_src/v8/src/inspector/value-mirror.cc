// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "src/v8/src/inspector/value-mirror.cc"

#include "brave/components/brave_page_graph/common/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#include "brave/v8/include/v8-isolate-page-graph-utils.h"
#include "v8-local-handle.h"
#include "v8-primitive.h"

// This file contains the implementation of the `SerializeValue` function. This
// function is used in PageGraph for the serialization of JavaScript objects,
// including those with non-enumerable and internal properties. To handle all
// potential edge cases, such as invoking getter functions, incorporating
// try/catch blocks, and appropriately responding to exceptions, the
// `SerializeValue` function leverages the existing ValueMirror machinery used
// by the Inspector Protocol.

namespace v8::page_graph {

namespace {

class PropertyMirrors : public v8_inspector::ValueMirror::PropertyAccumulator {
 public:
  PropertyMirrors() {}
  bool Add(v8_inspector::PropertyMirror mirror) override {
    m_mirrors_.push_back(std::move(mirror));
    return true;
  }

  auto& mirrors() { return m_mirrors_; }

 private:
  std::vector<v8_inspector::PropertyMirror> m_mirrors_;
};

// Returns true if the value is an empty object.
bool IsEmptyObject(Isolate* isolate, Local<Value> value) {
  if (!value->IsObject()) {
    return false;
  }
  auto obj = value.As<Object>();
  Local<Array> value_members =
      obj->GetPropertyNames(isolate->GetCurrentContext()).ToLocalChecked();
  return value_members->Length() == 0;
}

// Returns true if the object is a Node object.
bool IsNodeObject(Isolate* isolate, Local<Object> obj) {
  Local<Context> context = isolate->GetCurrentContext();
  while (!obj->IsNull()) {
    Local<Value> prototype = obj->GetPrototype();
    String::Utf8Value prototype_str(isolate, prototype);
    std::string_view prototype_string(*prototype_str, prototype_str.length());

    if (prototype_string == "[object Node]") {
      return true;
    }

    if (prototype->IsObject()) {
      obj = prototype->ToObject(context).ToLocalChecked();
    } else {
      break;
    }
  }
  return false;
}

Local<Value> ConvertProtocolValueToV8Value(
    Isolate* isolate,
    v8_inspector::protocol::Value* value) {
  if (!value) {
    return Undefined(isolate);
  }

  switch (value->type()) {
    case v8_inspector::protocol::Value::ValueType::TypeNull: {
      return Null(isolate);
    }
    case v8_inspector::protocol::Value::ValueType::TypeBoolean: {
      bool val;
      value->asBoolean(&val);
      return Number::New(isolate, val);
    }
    case v8_inspector::protocol::Value::ValueType::TypeInteger: {
      int val;
      value->asInteger(&val);
      return Number::New(isolate, val);
    }
    case v8_inspector::protocol::Value::ValueType::TypeDouble: {
      double val;
      value->asDouble(&val);
      return Number::New(isolate, val);
    }
    case v8_inspector::protocol::Value::ValueType::TypeString: {
      v8_inspector::protocol::String val;
      value->asString(&val);
      return v8_inspector::toV8String(isolate, val);
    }
    case v8_inspector::protocol::Value::ValueType::TypeBinary: {
      v8_inspector::protocol::Binary val;
      value->asBinary(&val);
      return v8_inspector::toV8String(isolate, val.toBase64());
    }
    case v8_inspector::protocol::Value::ValueType::TypeObject: {
      auto* dict = static_cast<v8_inspector::protocol::DictionaryValue*>(value);
      Local<Object> obj = Object::New(isolate);
      for (size_t i = 0; i < dict->size(); ++i) {
        const auto& entry = dict->at(i);
        Local<String> key = v8_inspector::toV8String(isolate, entry.first);
        Local<Value> value =
            ConvertProtocolValueToV8Value(isolate, entry.second);
        if (IsEmptyObject(isolate, value)) {
          continue;
        }
        obj->Set(isolate->GetCurrentContext(), key, value).Check();
      }
      return obj;
    }
    case v8_inspector::protocol::Value::ValueType::TypeArray: {
      auto* list = static_cast<v8_inspector::protocol::ListValue*>(value);
      Local<Array> arr =
          Array::New(isolate, static_cast<uint32_t>(list->size()));
      for (size_t i = 0; i < list->size(); ++i) {
        Local<Value> value =
            ConvertProtocolValueToV8Value(isolate, list->at(i));
        arr->Set(isolate->GetCurrentContext(), static_cast<uint32_t>(i), value)
            .Check();
      }
      return arr;
    }
    case v8_inspector::protocol::Value::ValueType::TypeImported: {
      return String::NewFromUtf8(isolate, "Imported").ToLocalChecked();
    }
  }
}

v8::Local<v8::Value> SerializeValue(v8::Local<v8::Context> context,
                                    v8::Local<v8::Value> value,
                                    int max_depth) {
  Local<Object> serialized_value;
  if (!max_depth--) {
    return serialized_value;
  }

  // Get all properties, including not enumerable and internal.
  PropertyMirrors properties;
  if (!v8_inspector::ValueMirror::getProperties(
          context, value.As<Object>(), false, false, false, &properties)) {
    return serialized_value;
  }

  Isolate* isolate = context->GetIsolate();
  serialized_value = Object::New(isolate);

  for (auto& mirror : properties.mirrors()) {
    if (!mirror.value) {
      continue;
    }

    Local<String> prop_name = v8_inspector::toV8String(isolate, mirror.name);
    Local<Value> prop_value =
        static_cast<v8_inspector::ValueMirrorBase*>(mirror.value.get())
            ->v8Value(isolate);

    // Skip function-like properties.
    if (prop_value->IsFunction()) {
      continue;
    }

    if (prop_value->IsObject()) {
      // Skip Node objects to avoid dumping the whole DOM as innerHTML.
      if (IsNodeObject(isolate, prop_value.As<Object>())) {
        continue;
      }
      prop_value = SerializeValue(context, prop_value, max_depth);
    } else {
      // Serialize non-object properties using Inspector Protocol internals.
      v8_inspector::WrapOptions wrap_options;
      wrap_options.mode = v8_inspector::WrapMode::kJson;
      wrap_options.serializationOptions.maxDepth = max_depth;
      std::unique_ptr<v8_inspector::protocol::Runtime::RemoteObject> result;

      if (!mirror.value->buildRemoteObject(context, wrap_options, &result)
               .IsSuccess()) {
        continue;
      }

      if (result->hasValue()) {
        prop_value = ConvertProtocolValueToV8Value(context->GetIsolate(),
                                                   result->getValue(nullptr));
      } else if (result->hasUnserializableValue()) {
        prop_value = v8_inspector::toV8String(
            isolate,
            result->getUnserializableValue(v8_inspector::protocol::String()));
      } else if (result->hasDescription()) {
        prop_value = v8_inspector::toV8String(
            isolate, result->getDescription(v8_inspector::protocol::String()));
      } else {
        // If the property is not serializable, it will be skipped.
        continue;
      }
    }

    // Skip empty properties.
    if (prop_value.IsEmpty() || IsEmptyObject(isolate, prop_value)) {
      continue;
    }

    serialized_value->Set(context, prop_name, prop_value).Check();
  }

  return serialized_value;
}

}  // namespace

v8::Local<v8::Value> SerializeValue(v8::Local<v8::Context> context,
                                    v8::Local<v8::Value> value) {
  // Limit max depth levels to cover most structs such as navigator.plugins, but
  // don't generate too much data in case a cycle is present. This is a
  // heuristic value and can be fine tuned if needed.
  return SerializeValue(context, value, 4);
}

}  // namespace v8::page_graph

#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
