/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BROWSER_BRAVE_ADS_BACKGROUND_HELPER_MAC_H_
#define BRAVE_COMPONENTS_BROWSER_BRAVE_ADS_BACKGROUND_HELPER_MAC_H_

#include "base/compiler_specific.h"
#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "brave/components/brave_ads/browser/background_helper.h"

@class BackgroundHelperDelegate;

namespace brave_ads {

class BackgroundHelperMac : public BackgroundHelper {
 public:
  BackgroundHelperMac();
  ~BackgroundHelperMac() override;

  static BackgroundHelperMac* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<BackgroundHelperMac>;

  // BackgroundHelper impl
  bool IsForeground() const override;

  base::scoped_nsobject<BackgroundHelperDelegate> delegate_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundHelperMac);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BROWSER_BRAVE_ADS_BACKGROUND_HELPER_MAC_H_
