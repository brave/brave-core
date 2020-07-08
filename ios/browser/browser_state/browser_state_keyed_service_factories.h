// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_IOS_BROWSER_BROWSER_STATE_BROWSER_STATE_KEYED_SERVICE_FACTORIES_H_
#define BRAVE_IOS_BROWSER_BROWSER_STATE_BROWSER_STATE_KEYED_SERVICE_FACTORIES_H_

// Instantiates all KeyedService factories which is especially important for
// services that register preferences or that should be created at browser
// state creation time (as opposed to lazily on first access).
void EnsureBrowserStateKeyedServiceFactoriesBuilt();

#endif  // BRAVE_IOS_BROWSER_BROWSER_STATE_BROWSER_STATE_KEYED_SERVICE_FACTORIES_H_
