/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_GEMINI_BROWSER_GEMINI_SERVICE_H_
#define BRAVE_COMPONENTS_GEMINI_BROWSER_GEMINI_SERVICE_H_

#include "base/memory/weak_ptr.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;

class GeminiService : public KeyedService {
 public:
  explicit GeminiService(content::BrowserContext* context);
  ~GeminiService() override;

 private:
  content::BrowserContext* context_;
  base::WeakPtrFactory<GeminiService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(GeminiService);
};

#endif  // BRAVE_COMPONENTS_GEMINI_BROWSER_GEMINI_SERVICE_H_
