/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/content/browser/cosmetic_filters_communication_impl.h"

#include "base/json/json_reader.h"
#include "brave/content/browser/cosmetic_filters_observer.h"
#include "content/public/browser/render_frame_host.h"

namespace content {

// static
void CosmeticFiltersCommunicationImpl::CreateInstance(
    content::RenderFrameHost* render_frame_host,
    CosmeticFiltersObserver* cosmetic_filters_observer) {
  if (!render_frame_host->cosmetic_filters_communication_impl_) {
    render_frame_host->cosmetic_filters_communication_impl_.reset(
        new CosmeticFiltersCommunicationImpl(
            render_frame_host, cosmetic_filters_observer));
  } else {
    render_frame_host->cosmetic_filters_communication_impl_->SetObserver(
        cosmetic_filters_observer);
  }
}

CosmeticFiltersCommunicationImpl::CosmeticFiltersCommunicationImpl(
    content::RenderFrameHost* render_frame_host,
    CosmeticFiltersObserver* cosmetic_filters_observer)
    : render_frame_host_(render_frame_host),
    cosmetic_filters_observer_(cosmetic_filters_observer) {
}

CosmeticFiltersCommunicationImpl::~CosmeticFiltersCommunicationImpl() {
}

void CosmeticFiltersCommunicationImpl::SetObserver(
    CosmeticFiltersObserver* cosmetic_filters_observer) {
  if (cosmetic_filters_observer && !cosmetic_filters_observer_) {
    cosmetic_filters_observer_ = cosmetic_filters_observer;
  }
}

void CosmeticFiltersCommunicationImpl::HiddenClassIdSelectors(
	  const std::string& input) {
  base::Optional<base::Value> input_value = base::JSONReader::Read(input);
  if (!input_value || !input_value->is_dict()) {
  	// Nothing to work with
  	return;
  }
  base::DictionaryValue* input_dict;
  if (!input_value->GetAsDictionary(&input_dict)) {
  	return;
  }
  std::vector<std::string> classes;
  base::ListValue* classes_list;
  if (input_dict->GetList("classes", &classes_list)) {
    for (size_t i = 0; i < classes_list->GetSize(); i++) {
      if (!classes_list->GetList()[i].is_string()) {
        continue;
      }
      classes.push_back(classes_list->GetList()[i].GetString());
    }
  }
  std::vector<std::string> ids;
  base::ListValue* ids_list;
  if (input_dict->GetList("ids", &ids_list)) {
    for (size_t i = 0; i < ids_list->GetSize(); i++) {
      if (!ids_list->GetList()[i].is_string()) {
        continue;
      }
      ids.push_back(ids_list->GetList()[i].GetString());
    }
  }
  if (cosmetic_filters_observer_) {
    cosmetic_filters_observer_->HiddenClassIdSelectors(render_frame_host_,
        classes, ids);
  }
}

}  // namespace content
