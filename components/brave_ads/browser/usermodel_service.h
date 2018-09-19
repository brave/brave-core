/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_USERMODEL_USERMODEL_SERVICE_H_
#define BRAVE_BROWSER_BRAVE_USERMODEL_USERMODEL_SERVICE_H_

#include "components/keyed_service/core/keyed_service.h"
#include "components/sessions/core/session_id.h"
#include "chrome/browser/profiles/profile.h"
#include "base/files/file_path.h"
#include "base/observer_list.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"

#include "user_model.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace brave_ads {

class UsermodelService : public KeyedService,
                         public base::SupportsWeakPtr<UsermodelService> {
 public:
    UsermodelService(Profile* profile);
    ~UsermodelService() override;

    void OnModelLoaded(const std::string& data);

    usermodel::UserModel usermodel_;

 private:
   const scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
};
}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_USERMODEL_USERMODEL_SERVICE_H_ 
