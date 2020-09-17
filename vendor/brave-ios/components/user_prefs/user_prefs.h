//
//  user_prefs.h
//  Sources
//
//  Created by brandon on 2020-05-08.
//

#ifndef user_prefs_h
#define user_prefs_h

#include "base/macros.h"
#include "base/supports_user_data.h"

class PrefService;

namespace user_prefs {
class UserPrefs : public base::SupportsUserData::Data {
 public:
  ~UserPrefs() override;
    
  static PrefService* Get(base::SupportsUserData* context);
    
  static void Set(base::SupportsUserData* context, PrefService* prefs);

 private:
  explicit UserPrefs(PrefService* prefs);
    
  PrefService* prefs_;

  DISALLOW_COPY_AND_ASSIGN(UserPrefs);
};

}  // namespace user_prefs

#endif /* user_prefs_h */
