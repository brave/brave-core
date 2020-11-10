/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CONTENT_BROWSER_COSMETIC_COMMUNICATION_IMPL_H_
#define BRAVE_CONTENT_BROWSER_COSMETIC_COMMUNICATION_IMPL_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/content/browser/mojom/cosmetic_filters_communication.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

namespace content {
class RenderFrameHost;
class CosmeticFiltersObserver;

class CosmeticFiltersCommunicationImpl final
	: public cf_comm::mojom::CosmeticFiltersCommunication {
 public:
  static void CreateInstance(content::RenderFrameHost* render_frame_host,
      CosmeticFiltersObserver* cosmetic_filters_observer);

  CosmeticFiltersCommunicationImpl(
   	   content::RenderFrameHost* render_frame_host,
       CosmeticFiltersObserver* cosmetic_filters_observer);
  ~CosmeticFiltersCommunicationImpl() override;

   // cf_comm::mojom::CosmeticFiltersCommunication
   void HiddenClassIdSelectors(const std::string& input) override;

   void SetObserver(CosmeticFiltersObserver* cosmetic_filters_observer);

 private:
   content::RenderFrameHost* render_frame_host_;
   CosmeticFiltersObserver* cosmetic_filters_observer_;
};

}  // namespace content

#endif  // BRAVE_CONTENT_BROWSER_COSMETIC_COMMUNICATION_IMPL_H_
