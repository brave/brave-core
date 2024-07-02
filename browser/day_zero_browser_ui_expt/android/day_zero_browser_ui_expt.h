/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_DAY_ZERO_BROWSER_UI_EXPT_DAY_ZERO_BROWSER_UI_EXPT_H_
#define BRAVE_BROWSER_DAY_ZERO_BROWSER_UI_EXPT_DAY_ZERO_BROWSER_UI_EXPT_H_

#include <memory>
#include <string>

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/day_zero_browser_ui_expt/common/mojom/day_zero.mojom.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace day_zero {

class DayZeroBrowserUiExpt : public mojom::DayZeroBrowserUiExpt {
 public:
  explicit DayZeroBrowserUiExpt(content::BrowserContext* context);
  ~DayZeroBrowserUiExpt() override;

  // mojom::DayZeroBrowserUiExpt:
  void IsDayZeroExpt(IsDayZeroExptCallback callback) override;

  void BindInterface(
      mojo::PendingReceiver<mojom::DayZeroBrowserUiExpt> pending_receiver);

  void Destroy(JNIEnv* env);
  jlong GetInterfaceToAndroidHelper(JNIEnv* env);

 private:
  mojo::ReceiverSet<mojom::DayZeroBrowserUiExpt> receivers_;
  base::WeakPtrFactory<DayZeroBrowserUiExpt> weak_ptr_factory_{this};
};

}  // namespace day_zero

#endif  // BRAVE_BROWSER_DAY_ZERO_BROWSER_UI_EXPT_DAY_ZERO_BROWSER_UI_EXPT_H_
