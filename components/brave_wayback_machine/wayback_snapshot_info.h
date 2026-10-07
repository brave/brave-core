/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_WAYBACK_SNAPSHOT_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_WAYBACK_SNAPSHOT_INFO_H_

#include "base/time/time.h"
#include "url/gurl.h"

// Describes a Wayback Machine snapshot of a page.
struct WaybackSnapshotInfo {
  friend bool operator==(const WaybackSnapshotInfo&,
                         const WaybackSnapshotInfo&) = default;

  // The https URL of the snapshot on the Wayback Machine host.
  GURL url;

  // The time the snapshot was taken. Null if unknown.
  base::Time time;
};

#endif  // BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_WAYBACK_SNAPSHOT_INFO_H_
