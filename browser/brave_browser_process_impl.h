// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_
#define BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_

#include "chrome/browser/browser_process_impl.h"

namespace brave_shields {
class BaseBraveShieldsService;
class HTTPSEverywhereService;
}

class BraveBrowserProcessImpl : public BrowserProcessImpl {
 public:
  BraveBrowserProcessImpl(base::SequencedTaskRunner* local_state_task_runner,
                     const base::CommandLine& command_line);
  ~BraveBrowserProcessImpl() override;


  brave_shields::BaseBraveShieldsService* ad_block_service();
  brave_shields::BaseBraveShieldsService*
      tracking_protection_service();
  brave_shields::HTTPSEverywhereService*
      https_everywhere_service();

 protected:
  void PreMainMessageLoopRun() override;

 private:
  std::unique_ptr<brave_shields::BaseBraveShieldsService> ad_block_service_;
  std::unique_ptr<brave_shields::BaseBraveShieldsService>
      tracking_protection_service_;
  std::unique_ptr<brave_shields::HTTPSEverywhereService>
      https_everywhere_service_;

  DISALLOW_COPY_AND_ASSIGN(BraveBrowserProcessImpl);
};

extern BraveBrowserProcessImpl* g_brave_browser_process;

#endif  // BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_
