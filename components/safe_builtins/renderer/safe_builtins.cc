/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/safe_builtins/renderer/safe_builtins.h"

#include <cstddef>
#include <string>
#include <tuple>

#include "base/containers/heap_array.h"
#include "base/notreached.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-exception.h"
#include "v8/include/v8-function.h"
#include "v8/include/v8-isolate.h"
#include "v8/include/v8-microtask-queue.h"
#include "v8/include/v8-object.h"
#include "v8/include/v8-primitive-object.h"
#include "v8/include/v8-primitive.h"
#include "v8/include/v8-script.h"

namespace brave {

namespace {

constexpr char kClassName[] = "brave::SafeBuiltins";

// see //extensions/renderer/safe_builtins.cc for the JS-side algorithm this
// mirrors. |Apply| and |Save| are passed in as ordinary function arguments and
// this is compiled and run directly, once per context, by EnsureSetUp() below.
constexpr char kScript[] =
    "(function(Apply, Save) {\n"
    "'use strict';\n"
    "\n"
    "// Used in the callback implementation, could potentially be clobbered.\n"
    "function makeCallable(obj, target, isStatic, propertyNames) {\n"
    "  propertyNames.forEach(function(propertyName) {\n"
    "    var property = obj[propertyName];\n"
    "    target[propertyName] = function() {\n"
    "      var recv = obj;\n"
    "      var firstArgIndex = 0;\n"
    "      if (!isStatic) {\n"
    "        if (arguments.length == 0)\n"
    "          throw 'There must be at least one argument, the receiver';\n"
    "        recv = arguments[0];\n"
    "        firstArgIndex = 1;\n"
    "      }\n"
    "      return Apply(\n"
    "          property, recv, arguments, firstArgIndex, arguments.length);\n"
    "    };\n"
    "  });\n"
    "}\n"
    "\n"
    "function saveBuiltin(builtin, protoPropertyNames, staticPropertyNames) {\n"
    "  var safe = function() {\n"
    "    throw 'Safe objects cannot be called nor constructed. ' +\n"
    "          'Use $Foo.self() or new $Foo.self() instead.';\n"
    "  };\n"
    "  safe.self = builtin;\n"
    "  makeCallable(builtin.prototype, safe, false, protoPropertyNames);\n"
    "  if (staticPropertyNames)\n"
    "    makeCallable(builtin, safe, true, staticPropertyNames);\n"
    "  Save(builtin.name, safe);\n"
    "}\n"
    "\n"
    "// Save only what is needed by wallet scripts.\n"
    "saveBuiltin(Object,\n"
    "            ['hasOwnProperty'],\n"
    "            ['create', 'defineProperty', 'freeze',\n"
    "             'getOwnPropertyDescriptor', 'getPrototypeOf', 'keys',\n"
    "             'assign', 'setPrototypeOf', 'defineProperties',\n"
    "             'entries']);\n"
    "saveBuiltin(Function,\n"
    "            ['apply', 'bind', 'call']);\n"
    "saveBuiltin(Array,\n"
    "            ['concat', 'forEach', 'indexOf', 'join', 'push', 'slice',\n"
    "             'splice', 'map', 'filter', 'shift', 'unshift', 'pop',\n"
    "             'reverse'],\n"
    "            ['from', 'isArray', 'of']);\n"
    "Save('$', function (value) { return value; })\n"
    "\n"
    "})";

// Converts |str| to a V8 string.
// This crashes when strlen(str) > v8::String::kMaxLength.
inline v8::Local<v8::String> ToV8StringUnsafe(
    v8::Isolate* isolate,
    const char* str,
    v8::NewStringType string_type = v8::NewStringType::kNormal) {
  DCHECK(strlen(str) <= v8::String::kMaxLength);
  return v8::String::NewFromUtf8(isolate, str, string_type).ToLocalChecked();
}

inline v8::Local<v8::String> ToV8StringUnsafe(
    v8::Isolate* isolate,
    const std::string& str,
    v8::NewStringType string_type = v8::NewStringType::kNormal) {
  return ToV8StringUnsafe(isolate, str.c_str(), string_type);
}

// Returns true if |maybe| is both a value, and that value is true.
inline bool IsTrue(v8::Maybe<bool> maybe) {
  return maybe.IsJust() && maybe.FromJust();
}

v8::Local<v8::Private> MakeKey(const char* name, v8::Isolate* isolate) {
  return v8::Private::ForApi(
      isolate,
      ToV8StringUnsafe(isolate, absl::StrFormat("%s::%s", kClassName, name)));
}

// GetProperty() family calls V8::Object::Get() and extracts a value from
// returned MaybeLocal. Returns true on success.
// NOTE: Think about whether you want this or GetPrivateProperty() below.
template <typename Key>
inline bool GetProperty(v8::Local<v8::Context> context,
                        v8::Local<v8::Object> object,
                        Key key,
                        v8::Local<v8::Value>* out) {
  return object->Get(context, key).ToLocal(out);
}

void SaveImpl(const char* name,
              v8::Local<v8::Value> value,
              v8::Local<v8::Context> context) {
  CHECK(!value.IsEmpty() && value->IsObject()) << std::string(name);
  context->Global()
      ->SetPrivate(context, MakeKey(name, v8::Isolate::GetCurrent()), value)
      .FromJust();
}

v8::Local<v8::Object> Load(const char* name, v8::Local<v8::Context> context) {
  v8::Local<v8::Value> value =
      context->Global()
          ->GetPrivate(context, MakeKey(name, v8::Isolate::GetCurrent()))
          .ToLocalChecked();
  CHECK(value->IsObject()) << std::string(name);
  return v8::Local<v8::Object>::Cast(value);
}

// The setup script always Save()s '$' last, so its presence indicates setup
// has already run for |context|.
bool HasBeenSetUp(v8::Local<v8::Context> context) {
  v8::Local<v8::Value> value;
  return context->Global()
             ->GetPrivate(context, MakeKey("$", v8::Isolate::GetCurrent()))
             .ToLocal(&value) &&
         !value->IsUndefined();
}

void Apply(const v8::FunctionCallbackInfo<v8::Value>& info) {
  CHECK(info.Length() == 5 &&
        info[0]->IsFunction() &&  // function
                                  // info[1] could be an object or a string
        info[2]->IsObject() &&    // args
        info[3]->IsInt32() &&     // first_arg_index
        info[4]->IsInt32());      // args_length
  v8::Local<v8::Context> context =
      v8::Isolate::GetCurrent()->GetCurrentContext();
  v8::MicrotasksScope microtasks(v8::Isolate::GetCurrent(),
                                 context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Local<v8::Function> function = info[0].As<v8::Function>();
  v8::Local<v8::Object> recv;
  if (info[1]->IsObject()) {
    recv = v8::Local<v8::Object>::Cast(info[1]);
  } else if (info[1]->IsString()) {
    recv = v8::StringObject::New(v8::Isolate::GetCurrent(),
                                 v8::Local<v8::String>::Cast(info[1]))
               .As<v8::Object>();
  } else {
    v8::Isolate::GetCurrent()->ThrowException(
        v8::Exception::TypeError(ToV8StringUnsafe(
            v8::Isolate::GetCurrent(),
            "The first argument is the receiver and must be an object")));
    return;
  }
  v8::Local<v8::Object> args = v8::Local<v8::Object>::Cast(info[2]);
  int first_arg_index = info[3].As<v8::Int32>()->Value();
  int args_length = info[4].As<v8::Int32>()->Value();

  int argc = args_length - first_arg_index;
  auto argv = base::HeapArray<v8::Local<v8::Value>>::WithSize(argc);
  for (int i = 0; i < argc; ++i) {
    CHECK(IsTrue(args->Has(context, i + first_arg_index)));
    // Getting a property value could throw an exception.
    if (!GetProperty(context, args, i + first_arg_index, &argv[i])) {
      return;
    }
  }

  v8::Local<v8::Value> return_value;
  if (function->Call(context, recv, argc, argv.data()).ToLocal(&return_value)) {
    info.GetReturnValue().Set(return_value);
  }
}

void Save(const v8::FunctionCallbackInfo<v8::Value>& info) {
  CHECK(info.Length() == 2 && info[0]->IsString() && info[1]->IsObject());
  SaveImpl(*v8::String::Utf8Value(v8::Isolate::GetCurrent(), info[0]), info[1],
           v8::Isolate::GetCurrent()->GetCurrentContext());
}

// Compiles and runs |kScript| directly in |context|, the first time a
// SafeBuiltins is created for it, so that Load() below has something to find.
void EnsureSetUp(v8::Local<v8::Context> context) {
  if (HasBeenSetUp(context)) {
    return;
  }

  v8::Isolate* const isolate = v8::Isolate::GetCurrent();
  v8::Context::Scope context_scope(context);
  v8::TryCatch try_catch(isolate);

  v8::ScriptCompiler::Source source(ToV8StringUnsafe(isolate, kScript));
  v8::Local<v8::Script> script;
  if (!v8::ScriptCompiler::Compile(
           context, &source, v8::ScriptCompiler::kNoCompileOptions,
           v8::ScriptCompiler::NoCacheReason::kNoCacheBecauseInlineScript)
           .ToLocal(&script)) {
    DUMP_WILL_BE_NOTREACHED() << "Failed to compile safe_builtins script";
    return;
  }

  v8::Local<v8::Value> func_as_value;
  if (!script->Run(context).ToLocal(&func_as_value) ||
      !func_as_value->IsFunction()) {
    DUMP_WILL_BE_NOTREACHED()
        << "safe_builtins script did not return a function";
    return;
  }

  v8::Local<v8::Function> apply_fn =
      v8::Function::New(context, Apply).ToLocalChecked();
  v8::Local<v8::Function> save_fn =
      v8::Function::New(context, Save).ToLocalChecked();
  v8::Local<v8::Value> args[] = {apply_fn, save_fn};

  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  std::ignore = func_as_value.As<v8::Function>()->Call(
      context, context->Global(), std::size(args), args);
}

}  // namespace

SafeBuiltins::SafeBuiltins(const v8::Local<v8::Context>& context)
    : context_(context) {
  EnsureSetUp(context_);
}

SafeBuiltins::~SafeBuiltins() {}

v8::Local<v8::Object> SafeBuiltins::GetObjekt() const {
  return Load("Object", context_);
}

v8::Local<v8::Object> SafeBuiltins::GetFunction() const {
  return Load("Function", context_);
}

v8::Local<v8::Object> SafeBuiltins::GetFunctionOverride() const {
  return Load("$", context_);
}

v8::Local<v8::Object> SafeBuiltins::GetArray() const {
  return Load("Array", context_);
}
}  //  namespace brave
