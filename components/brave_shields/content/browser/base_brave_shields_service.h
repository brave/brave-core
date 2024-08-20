// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_BASE_BRAVE_SHIELDS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_BASE_BRAVE_SHIELDS_SERVICE_H_

#include <stdint.h>

#include <string>
// TODO(brave): <mutex> is an unapproved C++11 header
#include <mutex>  // NOLINT

#include "base/task/sequenced_task_runner.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "url/gurl.h"

namespace brave_shields {

// The brave shields service in charge of checking brave shields like ad-block,
// tracking protection, etc.
class BaseBraveShieldsService {
 public:
  explicit BaseBraveShieldsService(
      scoped_refptr<base::SequencedTaskRunner> task_runner);
  virtual ~BaseBraveShieldsService();
  bool Start();
  bool IsInitialized() const;
  virtual void ShouldStartRequest(const GURL& url,
                                  blink::mojom::ResourceType resource_type,
                                  const std::string& tab_host,
                                  bool aggressive_blocking,
                                  bool* did_match_rule,
                                  bool* did_match_exception,
                                  bool* did_match_important,
                                  std::string* mock_data_url);

  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner();

 protected:
  virtual bool Init() = 0;

 private:
  void InitShields();

  bool initialized_;
  std::mutex initialized_mutex_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_BASE_BRAVE_SHIELDS_SERVICE_H_
