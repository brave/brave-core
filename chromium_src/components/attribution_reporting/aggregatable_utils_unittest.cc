/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// AggregatableUtilsNullReportsTest suite uses kNullReportsTestCases array where
// test cases are being initialized by dereferencing the return value of
// AggregatableTriggerConfig::Create  which we override to return nullopt. This
// causes the entire executable to crash on startup. To prevent crashing
// override the entire file.
