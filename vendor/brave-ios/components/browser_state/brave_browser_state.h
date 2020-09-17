//
//  BraveBrowserState.hpp
//  Sources
//
//  Created by brandon on 2020-05-06.
//

#ifndef BraveBrowserState_h
#define BraveBrowserState_h

#include <memory>

#include "base/macros.h"
#include "base/files/file_path.h"
#include "brave/vendor/brave-ios/components/browser_state/BraveBrowserStateIOData.h"

namespace sync_preferences {
class PrefServiceSyncable;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace base {
class FilePath;
}

namespace bookmarks {
class BookmarkModel;
}

namespace brave {
class BraveBrowserState : public ChromeBrowserState {
public:
  ~BraveBrowserState() override;
  
  // ChromeBrowserState:
   ChromeBrowserState* GetOriginalChromeBrowserState() override;
   bool HasOffTheRecordChromeBrowserState() const override;
   ChromeBrowserState* GetOffTheRecordChromeBrowserState() override;
   void DestroyOffTheRecordChromeBrowserState() override;
//   PrefProxyConfigTracker* GetProxyConfigTracker() override;
   PrefService* GetPrefs() override;
   PrefService* GetOffTheRecordPrefs() override;
   BraveBrowserStateIOData* GetIOData() override;
//   void ClearNetworkingHistorySince(base::Time time,
//                                    const base::Closure& completion) override;

   // BrowserState:
   bool IsOffTheRecord() const override;
   base::FilePath GetStatePath() const override;

  private:
   friend class ChromeBrowserStateManagerImpl;

   BraveBrowserState(
       scoped_refptr<base::SequencedTaskRunner> io_task_runner,
       const base::FilePath& path);

   // Sets the OffTheRecordChromeBrowserState.
   void SetOffTheRecordChromeBrowserState(
       std::unique_ptr<ChromeBrowserState> otr_state);

   base::FilePath state_path_;
  
   std::unique_ptr<ChromeBrowserState> otr_state_;
   base::FilePath otr_state_path_;

   scoped_refptr<user_prefs::PrefRegistrySyncable> pref_registry_;
   std::unique_ptr<sync_preferences::PrefServiceSyncable> prefs_;
   std::unique_ptr<sync_preferences::PrefServiceSyncable> otr_prefs_;
   std::unique_ptr<BraveBrowserStateIOData::Handle> io_data_;

   std::unique_ptr<PrefProxyConfigTracker> pref_proxy_config_tracker_;
   DISALLOW_COPY_AND_ASSIGN(BraveBrowserState);
};
}

#endif /* BraveBrowserState_h */
