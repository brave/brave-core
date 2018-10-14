// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TOOLBAR_BRAVE_TOOLBAR_MODEL_IMPL_H_
#define BRAVE_COMPONENTS_TOOLBAR_BRAVE_TOOLBAR_MODEL_IMPL_H_

#include "components/toolbar/toolbar_model_impl.h"

class BraveToolbarModelImpl : public ToolbarModelImpl {
  public:
    using ToolbarModelImpl::ToolbarModelImpl;
    base::string16 GetURLForDisplay() const override;
  private:
    DISALLOW_COPY_AND_ASSIGN(BraveToolbarModelImpl);
};

#endif
