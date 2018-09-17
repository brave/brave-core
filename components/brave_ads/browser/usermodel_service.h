/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
 /* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_USERMODEL_USERMODEL_SERVICE_H_
#define BRAVE_BROWSER_BRAVE_USERMODEL_USERMODEL_SERVICE_H_

#include "components/keyed_service/core/keyed_service.h"
#include "components/sessions/core/session_id.h"

namespace brave_ads {
class UsermodelService : public KeyedService {
 public:
    UsermodelService();
    ~UsermodelService() override;


 private:
};
}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_USERMODEL_USERMODEL_SERVICE_H_ 
