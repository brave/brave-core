/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/renderer/blink_document_extractor.h"

#include <string>
#include <utility>

#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace web_discovery {

namespace {

mojom::AttributeResultPtr ProcessAttributeRequest(
    mojom::SelectAttributeRequest* request,
    const blink::WebVector<blink::WebElement>& elements) {
  auto attributes_result = mojom::AttributeResult::New();
  attributes_result->key = request->key;
  for (const auto& element : elements) {
    std::optional<blink::WebElement> sub_element;
    const auto* element_to_query = &element;
    if (request->sub_selector) {
      auto web_sub_selector =
          blink::WebString::FromUTF8(*request->sub_selector);
      sub_element = element.QuerySelector(web_sub_selector);
      element_to_query = &*sub_element;
    }
    std::string attribute_value;
    if (element_to_query->IsNull()) {
      continue;
    }
    auto attribute_name = blink::WebString::FromUTF8(request->attribute);
    auto web_attribute_value = element_to_query->GetAttribute(attribute_name);
    if (web_attribute_value.IsNull()) {
      continue;
    }
    attributes_result->attribute_values.push_back(web_attribute_value.Utf8());
  }
  return attributes_result;
}

}  // namespace

BlinkDocumentExtractor::BlinkDocumentExtractor(
    content::RenderFrame* render_frame,
    service_manager::BinderRegistry* registry)
    : content::RenderFrameObserver(render_frame), render_frame_(render_frame) {
  registry->AddInterface<mojom::DocumentExtractor>(base::BindRepeating(
      &BlinkDocumentExtractor::BindReceiver, base::Unretained(this)));
}

BlinkDocumentExtractor::~BlinkDocumentExtractor() = default;

void BlinkDocumentExtractor::QueryElementAttributes(
    std::vector<mojom::SelectRequestPtr> requests,
    QueryElementAttributesCallback callback) {
  blink::WebDocument document = render_frame_->GetWebFrame()->GetDocument();
  std::vector<mojom::AttributeResultPtr> results;
  for (const auto& request : requests) {
    auto result = mojom::AttributeResult::New();

    auto selector = blink::WebString::FromUTF8(request->root_selector);
    auto elements = document.QuerySelectorAll(selector);
    for (const auto& attribute_request : request->attribute_requests) {
      auto attributes_result =
          ProcessAttributeRequest(attribute_request.get(), elements);
      results.push_back(std::move(attributes_result));
    }
  }

  std::move(callback).Run(std::move(results));
}

void BlinkDocumentExtractor::OnDestruct() {
  delete this;
}

void BlinkDocumentExtractor::BindReceiver(
    mojo::PendingReceiver<mojom::DocumentExtractor> receiver) {
  receiver_.reset();
  receiver_.Bind(std::move(receiver));
}

}  // namespace web_discovery
