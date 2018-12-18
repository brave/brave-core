// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_LOCATION_BAR_MODEL_IMPL_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_LOCATION_BAR_MODEL_IMPL_H_

#include "components/omnibox/browser/location_bar_model_impl.h"

class BraveLocationBarModelImpl : public LocationBarModelImpl {
  public:
    using LocationBarModelImpl::LocationBarModelImpl;
    base::string16 GetURLForDisplay() const override;
  private:
    DISALLOW_COPY_AND_ASSIGN(BraveLocationBarModelImpl);
};

#endif
