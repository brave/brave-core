/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/content/browser/cosmetic_filters_communication_impl.h"

#include <vector>

#include "base/json/json_reader.h"
#include "base/values.h"
#include "brave/content/browser/cosmetic_filters_observer.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"

// TODO remove stub
#include "base/json/json_reader.h"

namespace content {

// static
void CosmeticFiltersCommunicationImpl::CreateInstance(
    content::RenderFrameHost* render_frame_host,
    CosmeticFiltersObserver* cosmetic_filters_observer) {
  if (!render_frame_host->cosmetic_filters_communication_impl_) {
    render_frame_host->cosmetic_filters_communication_impl_.reset(
        new CosmeticFiltersCommunicationImpl(render_frame_host,
                                             cosmetic_filters_observer));
  } else {
    render_frame_host->cosmetic_filters_communication_impl_->SetObserver(
        cosmetic_filters_observer);
  }
}

CosmeticFiltersCommunicationImpl::CosmeticFiltersCommunicationImpl(
    content::RenderFrameHost* render_frame_host,
    CosmeticFiltersObserver* cosmetic_filters_observer)
    : frame_id_(content::GlobalFrameRoutingId(
          render_frame_host->GetProcess()->GetID(),
          render_frame_host->GetRoutingID())),
      cosmetic_filters_observer_(cosmetic_filters_observer) {}

CosmeticFiltersCommunicationImpl::~CosmeticFiltersCommunicationImpl() {}

void CosmeticFiltersCommunicationImpl::SetObserver(
    CosmeticFiltersObserver* cosmetic_filters_observer) {
  if (cosmetic_filters_observer && !cosmetic_filters_observer_) {
    cosmetic_filters_observer_ = cosmetic_filters_observer;
  }
}

void CosmeticFiltersCommunicationImpl::ShouldDoCosmeticFiltering(
    const std::string& url,
    ShouldDoCosmeticFilteringCallback callback) {
  // TODO
  LOG(ERROR) << "!!!CosmeticFiltersCommunicationImpl::ShouldDoCosmeticFiltering";
  std::move(callback).Run(true, false);
}

void CosmeticFiltersCommunicationImpl::UrlCosmeticResources(
    const std::string& url,
    UrlCosmeticResourcesCallback callback) {
  // TODO remove stub
  base::Optional<base::Value> resources = base::JSONReader::Read("{}");
  std::move(callback).Run(resources->Clone());
}

void CosmeticFiltersCommunicationImpl::HiddenClassIdSelectors(
    const std::string& input,
    HiddenClassIdSelectorsCallback callback) {
  base::Optional<base::Value> resources = base::JSONReader::Read("{}");
  base::Optional<base::Value> input_value = base::JSONReader::Read(input);
  if (!input_value || !input_value->is_dict()) {
    // Nothing to work with
    std::move(callback).Run(resources->Clone());

    return;
  }
  base::DictionaryValue* input_dict;
  if (!input_value->GetAsDictionary(&input_dict)) {
    std::move(callback).Run(resources->Clone());

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
  // TODO remove stub
  std::move(callback).Run(resources->Clone());
  //auto* frame_host = content::RenderFrameHost::FromID(frame_id_);
  //if (cosmetic_filters_observer_ && frame_host) {
  //  cosmetic_filters_observer_->ApplyHiddenClassIdSelectors(frame_host, classes,
  //                                                          ids);
  //}
}

}  // namespace content
