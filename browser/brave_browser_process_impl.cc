// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/brave_browser_process_impl.h"

BraveBrowserProcessImpl::BraveBrowserProcessImpl(
    base::SequencedTaskRunner* local_state_task_runner,
    const base::CommandLine& command_line)
    : BrowserProcessImpl(local_state_task_runner, command_line) {
  g_browser_process = this;
}
