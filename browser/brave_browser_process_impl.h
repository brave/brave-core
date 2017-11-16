// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_
#define BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_

#include "chrome/browser/browser_process_impl.h"

class BraveBrowserProcessImpl : public BrowserProcessImpl {
 public:
  BraveBrowserProcessImpl(base::SequencedTaskRunner* local_state_task_runner,
                     const base::CommandLine& command_line);

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveBrowserProcessImpl);
};

#endif  // BRAVE_BROWSER_BRAVE_BROWSER_PROCESS_IMPL_H_
