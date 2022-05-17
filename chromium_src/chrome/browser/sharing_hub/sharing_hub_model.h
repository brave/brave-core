#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SHARING_HUB_SHARING_HUB_MODEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SHARING_HUB_SHARING_HUB_MODEL_H_

#define BRAVE_SHARING_HUB_MODEL_H \
private: \
    friend class BraveSharingHubModel;

#define GetFirstPartyActionList virtual GetFirstPartyActionList

#include "src/chrome/browser/sharing_hub/sharing_hub_model.h"

#undef GetFirstPartyActionList
#undef BRAVE_SHARING_HUB_MODEL_H

#endif // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SHARING_HUB_SHARING_HUB_MODEL_H_
