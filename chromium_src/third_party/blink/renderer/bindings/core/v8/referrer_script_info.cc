/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/bindings/core/v8/referrer_script_info.h"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#define FromV8HostDefinedOptions FromV8HostDefinedOptions_ChromiumImpl
#define ToV8HostDefinedOptions ToV8HostDefinedOptions_ChromiumImpl
#define BRAVE_REFERRER_SCRIPT_INFO_IS_DEFAULT_VALUE \
  (dom_node_id_ == kInvalidDOMNodeId && parent_script_id_ == 0)
#else
#define BRAVE_REFERRER_SCRIPT_INFO_IS_DEFAULT_VALUE true
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#include "src/third_party/blink/renderer/bindings/core/v8/referrer_script_info.cc"
#undef BRAVE_REFERRER_SCRIPT_INFO_IS_DEFAULT_VALUE
#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#undef ToV8HostDefinedOptions
#undef FromV8HostDefinedOptions

namespace blink {

static constexpr size_t kBraveAddedFieldsCount = 2;

static_assert(ReferrerScriptInfo::HostDefinedOptionsIndex::kLength ==
                  HostDefinedOptionsIndex::kLength + kBraveAddedFieldsCount,
              "Please sync blink::ReferrerScriptInfo::HostDefinedOptionsIndex "
              "with blink::HostDefinedOptionsIndex");

// static
ReferrerScriptInfo ReferrerScriptInfo::FromV8HostDefinedOptions(
    v8::Local<v8::Context> context,
    v8::Local<v8::Data> raw_host_defined_options,
    const KURL& script_origin_resource_name) {
  ReferrerScriptInfo script_info =
      ReferrerScriptInfo::FromV8HostDefinedOptions_ChromiumImpl(
          context, raw_host_defined_options, script_origin_resource_name);

  if (!raw_host_defined_options.IsEmpty() &&
      raw_host_defined_options->IsFixedArray()) {
    v8::Local<v8::PrimitiveArray> host_defined_options =
        v8::Local<v8::PrimitiveArray>::Cast(raw_host_defined_options);
    if (host_defined_options->Length()) {
      v8::Isolate* isolate = context->GetIsolate();

      v8::Local<v8::Primitive> dom_node_id_value = host_defined_options->Get(
          isolate, HostDefinedOptionsIndex::kDomNodeId);
      SECURITY_CHECK(dom_node_id_value->IsInt32());
      script_info.dom_node_id_ =
          dom_node_id_value->Int32Value(context).ToChecked();

      v8::Local<v8::Primitive> parent_script_id_value =
          host_defined_options->Get(isolate,
                                    HostDefinedOptionsIndex::kParentScriptId);
      SECURITY_CHECK(parent_script_id_value->IsInt32());
      script_info.parent_script_id_ =
          parent_script_id_value->Int32Value(context).ToChecked();
    }
  }

  return script_info;
}

v8::Local<v8::Data> ReferrerScriptInfo::ToV8HostDefinedOptions(
    v8::Isolate* isolate,
    const KURL& script_origin_resource_name) const {
  v8::Local<v8::Data> raw_host_defined_options =
      ToV8HostDefinedOptions_ChromiumImpl(isolate, script_origin_resource_name);
  if (!raw_host_defined_options.IsEmpty() &&
      raw_host_defined_options->IsFixedArray()) {
    v8::Local<v8::PrimitiveArray> host_defined_options =
        v8::Local<v8::PrimitiveArray>::Cast(raw_host_defined_options);
    DCHECK_EQ(
        host_defined_options->Length(),
        static_cast<int>(ReferrerScriptInfo::HostDefinedOptionsIndex::kLength));

    v8::Local<v8::Primitive> dom_node_id_value =
        v8::Integer::New(isolate, dom_node_id_);
    host_defined_options->Set(isolate, HostDefinedOptionsIndex::kDomNodeId,
                              dom_node_id_value);

    v8::Local<v8::Primitive> parent_script_id_value =
        v8::Integer::New(isolate, parent_script_id_);
    host_defined_options->Set(isolate, HostDefinedOptionsIndex::kParentScriptId,
                              parent_script_id_value);
  }

  return raw_host_defined_options;
}

}  // namespace blink

#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
