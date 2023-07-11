// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

declare namespace chrome.leo {
    const reset: (callback: (success: boolean) => void) => void
    const getShowLeoAssistantIcon:
        (callback: (success: boolean) => void) => boolean
    const setShowLeoAssistantIcon:
        (is_visible: boolean, callback: (success: boolean) => void) => void
}
