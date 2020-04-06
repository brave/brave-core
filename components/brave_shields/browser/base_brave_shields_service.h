/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BASE_BRAVE_SHIELDS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BASE_BRAVE_SHIELDS_SERVICE_H_

#include <stdint.h>

#include <memory>
#include <string>
// TODO(brave): <mutex> is an unapproved C++11 header
#include <mutex>  // NOLINT

#include "base/files/file_path.h"
#include "base/sequenced_task_runner.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "url/gurl.h"

using brave_component_updater::BraveComponent;

namespace brave_shields {

// The brave shields service in charge of checking brave shields like ad-block,
// tracking protection, etc.
class BaseBraveShieldsService : public BraveComponent {
 public:
  explicit BaseBraveShieldsService(BraveComponent::Delegate* delegate);
  ~BaseBraveShieldsService() override;
  bool Start();
  void Stop();
  bool IsInitialized() const;
  virtual bool ShouldStartRequest(const GURL& url,
                                  blink::mojom::ResourceType resource_type,
                                  const std::string& tab_host,
                                  bool* did_match_exception,
                                  bool* cancel_request_explicitly,
                                  std::string* mock_data_url);

 protected:
  virtual bool Init() = 0;
  virtual void Cleanup() = 0;

 private:
  void InitShields();

  bool initialized_;
  std::mutex initialized_mutex_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BASE_BRAVE_SHIELDS_SERVICE_H_
