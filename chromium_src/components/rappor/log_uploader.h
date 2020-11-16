/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_RAPPOR_LOG_UPLOADER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_RAPPOR_LOG_UPLOADER_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "components/rappor/log_uploader_interface.h"
#include "url/gurl.h"

#define Start Start_ChromiumImpl(); \
  void Start
#include "../../../../components/rappor/log_uploader.h"
#undef Start

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_RAPPOR_LOG_UPLOADER_H_
