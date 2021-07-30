// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SEARCH_BROWSER_BRAVE_SEARCH_DEFAULT_HOST_PRIVATE_H_
#define BRAVE_COMPONENTS_BRAVE_SEARCH_BROWSER_BRAVE_SEARCH_DEFAULT_HOST_PRIVATE_H_

#include "brave/components/brave_search/common/brave_search_default.mojom.h"

class TemplateURLService;
class PrefService;

namespace brave_search {

class BraveSearchDefaultHostPrivate final
    : public brave_search::mojom::BraveSearchDefault {
 public:
  BraveSearchDefaultHostPrivate(const BraveSearchDefaultHostPrivate&) = delete;
  BraveSearchDefaultHostPrivate& operator=(
      const BraveSearchDefaultHostPrivate&) = delete;

  BraveSearchDefaultHostPrivate() = default;
  ~BraveSearchDefaultHostPrivate() override;

  // brave_search::mojom::BraveSearchDefault:
  void GetCanSetDefaultSearchProvider(
      GetCanSetDefaultSearchProviderCallback callback) override;
  void SetIsDefaultSearchProvider() override;
};

}  // namespace brave_search

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_BROWSER_BRAVE_SEARCH_DEFAULT_HOST_PRIVATE_H_
