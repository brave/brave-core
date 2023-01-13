/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.features.ledger_hw;

import org.chromium.components.module_installer.builder.ModuleInterface;

/** Interface to call into DevUI feature. */
@ModuleInterface(module = "ledger_hw", impl = "org.chromium.chrome.features.LedgerHwImpl")
public interface LedgerHw {}
