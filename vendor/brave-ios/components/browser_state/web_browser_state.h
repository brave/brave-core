//
//  web_browser_state.hpp
//  base/third_party/double_conversion:double_conversion
//
//  Created by brandon on 2020-05-07.
//

#ifndef web_browser_state_h
#define web_browser_state_h

#include <memory>

#include "base/macros.h"
#include "base/supports_user_data.h"

namespace base {
class FilePath;
}

namespace web {
class BrowserState : public base::SupportsUserData {
 public:
  ~BrowserState() override;
    
  virtual bool IsOffTheRecord() const = 0;
  virtual base::FilePath GetStatePath() const = 0;
  static BrowserState* FromSupportsUserData(
      base::SupportsUserData* supports_user_data);

 protected:
  BrowserState();
};

}  // namespace web

#endif /* web_browser_state_h */
