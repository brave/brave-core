/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/utilities/digest.h"
#include <memory>
#include <string>
#include "third_party/blink/renderer/core/loader/resource/css_style_sheet_resource.h"
#include "third_party/blink/renderer/core/loader/resource/script_resource.h"
#include "third_party/blink/renderer/core/loader/resource/text_resource.h"
#include "third_party/blink/renderer/platform/crypto.h"
#include "third_party/blink/renderer/platform/graphics/image.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "third_party/blink/renderer/platform/wtf/text/base64.h"
#include "third_party/blink/renderer/platform/wtf/text/string_utf8_adaptor.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "brave/third_party/blink/brave_page_graph/logging.h"

using ::blink::ComputeDigest;
using ::blink::CSSStyleSheetResource;
using ::blink::DigestValue;
using ::blink::Image;
using ::blink::Resource;
using ::blink::ScriptResource;
using ::blink::SharedBuffer;
using ::std::make_unique;
using ::std::string;
using ::WTF::Base64Encode;
using ::WTF::SafeCast;

namespace brave_page_graph {

namespace {

void MaybeEncodeTextContent(const String& text_content, const char* buffer_data,
    wtf_size_t buffer_size, String* result, bool* base64_encoded) {
  if (!text_content.IsNull()) {
    *result = text_content;
    *base64_encoded = false;
  } else if (buffer_data) {
    *result =
        Base64Encode(base::as_bytes(base::make_span(buffer_data, buffer_size)));
    *base64_encoded = true;
  } else if (text_content.IsNull()) {
    *result = "";
    *base64_encoded = false;
  } else {
    DCHECK(!text_content.Is8Bit());
    *result = Base64Encode(
        base::as_bytes(base::make_span(StringUTF8Adaptor(text_content))));
    *base64_encoded = true;
  }
}

void MaybeEncodeTextContent(const WTF::String& text_content,
    scoped_refptr<const SharedBuffer> buffer, WTF::String* result,
    bool* base64_encoded) {
  if (!buffer) {
    return MaybeEncodeTextContent(text_content, nullptr, 0, result,
                                  base64_encoded);
  }

  const SharedBuffer::DeprecatedFlatData flat_buffer(std::move(buffer));
  return MaybeEncodeTextContent(text_content, flat_buffer.Data(),
                                SafeCast<wtf_size_t>(flat_buffer.size()),
                                result, base64_encoded);
}

}

string ImageDigest(Image* image) {
  DigestValue digest;

  const size_t size = image->Data()->size();
  auto data = make_unique<char[]>(size);
  if (image->Data()->GetBytes(data.get(), size)) {
    const bool digest_succeeded = ComputeDigest(blink::kHashAlgorithmSha256,
        data.get(), size, digest);
    LOG_ASSERT(digest_succeeded);
    return string(Base64Encode(digest).Utf8().data());
  }

  return string("");
}

string ScriptDigest(blink::ScriptResource* resource) {
  DigestValue digest;
  WTF::String result;
  bool base64_encoded;
  MaybeEncodeTextContent(
    resource->TextForInspector(),
    resource->ResourceBuffer(), &result, &base64_encoded);
  string str_copy(result.Utf8().data());
  const bool digest_succeeded = ComputeDigest(blink::kHashAlgorithmSha256,
      str_copy.c_str(), str_copy.size(), digest);
  LOG_ASSERT(digest_succeeded);
  return string(Base64Encode(digest).Utf8().data());
}

string StyleSheetDigest(CSSStyleSheetResource* resource) {
  DigestValue digest;
  WTF::String result;
  bool base64_encoded;
  MaybeEncodeTextContent(
      resource->SheetText(nullptr, CSSStyleSheetResource::MIMETypeCheck::kLax),
      resource->ResourceBuffer(), &result, &base64_encoded);
  string str_copy(result.Utf8().data());
  const bool digest_succeeded = ComputeDigest(blink::kHashAlgorithmSha256,
      str_copy.c_str(), str_copy.size(), digest);
  LOG_ASSERT(digest_succeeded);
  return string(Base64Encode(digest).Utf8().data());
}

string ResourceDigest(Resource* resource) {
  blink::ResourceType type = resource->GetType();
  if (type == blink::ResourceType::kCSSStyleSheet) {
    return StyleSheetDigest(ToCSSStyleSheetResource(resource));
  } else if (type == blink::ResourceType::kScript) {
    return ScriptDigest(ToScriptResource(resource));
  }

  DigestValue digest;
  if (!resource->ResourceBuffer()) {
    return "";
  }

  const size_t size = resource->ResourceBuffer()->size();
  auto data = make_unique<char[]>(size);
  if (resource->ResourceBuffer()->GetBytes(data.get(), size)) {
    const bool digest_succeeded = ComputeDigest(blink::kHashAlgorithmSha256,
        data.get(), size, digest);
    LOG_ASSERT(digest_succeeded);
    return string(Base64Encode(digest).Utf8().data());
  }

  return string("");
}

}  // namespace brave_page_graph