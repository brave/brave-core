// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_TRAFFIC_CONTROL_TRAFFIC_CONTROL_APPLY_H_
#define BRAVE_BROWSER_TRAFFIC_CONTROL_TRAFFIC_CONTROL_APPLY_H_

#include <optional>

#include "base/memory/weak_ptr.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"
#include "url/origin.h"

class BrowserWindowInterface;
class Profile;
class TabStripModel;

namespace containers::mojom {
class Container;
}  // namespace containers::mojom

namespace traffic_control {

// Applies a Traffic Control rule target: opens |url| in a new tab (optionally
// in a container), then closes an empty source tab when appropriate.
class TrafficControlApplier {
 public:
  TrafficControlApplier(content::WebContents* source,
                        const GURL& url,
                        mojom::TargetPtr target,
                        std::optional<url::Origin> initiator_origin,
                        ui::PageTransition page_transition);

  TrafficControlApplier(const TrafficControlApplier&) = delete;
  TrafficControlApplier& operator=(const TrafficControlApplier&) = delete;

  ~TrafficControlApplier();

  // Entry point used by the navigation throttle after a rule match is
  // cancelled.
  static void Apply(base::WeakPtr<content::WebContents> source,
                    const GURL& url,
                    mojom::TargetPtr target,
                    std::optional<url::Origin> initiator_origin,
                    ui::PageTransition page_transition);

  // Returns true when |web_contents| already satisfies |target| and re-routing
  // should not run. Unset destinations and unknown-container fallback to
  // non-contained tabs are treated as already at target.
  static bool AlreadyAtTarget(content::WebContents* web_contents,
                              const mojom::Target& target);

  // Resolves browser context and performs the re-route. No-op when inputs or
  // browser state cannot be resolved.
  void Run();

 private:
  bool Initialize();
  bool OpenTargetTab();
  void OpenUrl(const containers::mojom::Container* container);
  void MaybeCloseEmptySourceTab();

  raw_ptr<content::WebContents> source_;
  GURL url_;
  mojom::TargetPtr target_;
  std::optional<url::Origin> initiator_origin_;
  ui::PageTransition page_transition_;

  raw_ptr<BrowserWindowInterface> browser_window_ = nullptr;
  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<TabStripModel> tab_strip_ = nullptr;
  bool close_source_after_ = false;
};

}  // namespace traffic_control

#endif  // BRAVE_BROWSER_TRAFFIC_CONTROL_TRAFFIC_CONTROL_APPLY_H_
