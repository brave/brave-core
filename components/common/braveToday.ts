// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

export function isPublisherContentAllowed (publisher: BraveToday.Publisher): boolean {
  // Either the publisher is enabled-by-default (remotely) and the user has
  // not overriden that default, or the user has made a choice.
  if (publisher.user_enabled) {
    return true
  }
  if (publisher.enabled && publisher.user_enabled !== false) {
    return true
  }
  return false
}
