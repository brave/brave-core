/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/battery/battery_manager.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/modules/battery/battery_dispatcher.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

  BatteryManager::~BatteryManager() = default;

  BatteryManager::BatteryManager(ExecutionContext* context)
      : ContextLifecycleStateObserver(context),
        PlatformEventController(To<Document>(context)) {}

  BatteryManager* BatteryManager::Create(ExecutionContext* context) {
    BatteryManager* battery_manager =
    MakeGarbageCollected<BatteryManager>(context);
    battery_manager->UpdateStateIfNeeded();
    return battery_manager;
  }

  ScriptPromise BatteryManager::StartRequest(ScriptState* script_state) {
    if (!battery_property_) {
      battery_property_ = MakeGarbageCollected<BatteryProperty>(
        ExecutionContext::From(script_state), this, BatteryProperty::kReady);
      battery_property_->Resolve(this);
    }
    return battery_property_->Promise(script_state->World());
  }

  bool BatteryManager::charging() {
    return true;
  }

  double BatteryManager::chargingTime() {
    return 0;
  }

  double BatteryManager::dischargingTime() {
    return std::numeric_limits<double>::infinity();
  }

  double BatteryManager::level() {
    return 1.0;
  }

  void BatteryManager::Trace(blink::Visitor* visitor) {
    visitor->Trace(battery_property_);
    PlatformEventController::Trace(visitor);
    EventTargetWithInlineData::Trace(visitor);
    ContextLifecycleStateObserver::Trace(visitor);
  }

  bool BatteryManager::HasPendingActivity() const {
    return false;
  }

  void BatteryManager::ContextLifecycleStateChanged(
      mojom::FrameLifecycleState) {
    return;
  }

  void BatteryManager::UnregisterWithDispatcher() {
    return;
  }

  void BatteryManager::RegisterWithDispatcher() {
    return;
  }

  void BatteryManager::ContextDestroyed(ExecutionContext*) {
    battery_property_ = nullptr;
    return;
  }

  void BatteryManager::DidUpdateData() {
    return;
  }

  bool BatteryManager::HasLastData() {
    return false;
  }
}  // namespace blink
