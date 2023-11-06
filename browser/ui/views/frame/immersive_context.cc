/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/immersive_context.h"

#include "base/check_op.h"

namespace {

ImmersiveContext* g_instance = nullptr;

}  // namespace

ImmersiveContext::~ImmersiveContext() {
  DCHECK_EQ(this, g_instance);
  g_instance = nullptr;
}

// static
ImmersiveContext* ImmersiveContext::Get() {
  return g_instance;
}

ImmersiveContext::ImmersiveContext() {
  DCHECK_EQ(nullptr, g_instance);
  g_instance = this;
}
