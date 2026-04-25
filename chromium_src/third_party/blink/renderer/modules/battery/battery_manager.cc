/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/battery/battery_manager.h"

#include "third_party/blink/public/mojom/frame/lifecycle.mojom-blink.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/modules/battery/battery_dispatcher.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

const char BatteryManager::kSupplementName[] = "BatteryManager";

// static
ScriptPromise<BatteryManager> BatteryManager::getBattery(
    ScriptState* script_state,
    Navigator& navigator) {
  if (!navigator.DomWindow())
    return ScriptPromise<BatteryManager>();

  auto* supplement = Supplement<Navigator>::From<BatteryManager>(navigator);
  if (!supplement) {
    supplement = MakeGarbageCollected<BatteryManager>(navigator);
    ProvideTo(navigator, supplement);
  }
  return supplement->StartRequest(script_state);
}

BatteryManager::~BatteryManager() = default;

BatteryManager::BatteryManager(Navigator& navigator)
    : ActiveScriptWrappable<BatteryManager>({}),
      Supplement<Navigator>(navigator),
      ExecutionContextLifecycleStateObserver(navigator.DomWindow()),
      PlatformEventController(*navigator.DomWindow()),
      battery_dispatcher_(
          MakeGarbageCollected<BatteryDispatcher>(navigator.DomWindow())) {
  UpdateStateIfNeeded();
}

ScriptPromise<BatteryManager> BatteryManager::StartRequest(
    ScriptState* script_state) {
  if (!battery_property_) {
    battery_property_ = MakeGarbageCollected<BatteryProperty>(
        ExecutionContext::From(script_state));
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

void BatteryManager::DidUpdateData() {
  return;
}

void BatteryManager::RegisterWithDispatcher() {
  return;
}

void BatteryManager::UnregisterWithDispatcher() {
  return;
}

bool BatteryManager::HasLastData() {
  return false;
}

void BatteryManager::ContextLifecycleStateChanged(mojom::FrameLifecycleState) {
  return;
}

void BatteryManager::ContextDestroyed() {
  battery_property_ = nullptr;
  return;
}

bool BatteryManager::HasPendingActivity() const {
  return false;
}

void BatteryManager::Trace(blink::Visitor* visitor) const {
  visitor->Trace(battery_property_);
  visitor->Trace(battery_dispatcher_);
  Supplement<Navigator>::Trace(visitor);
  PlatformEventController::Trace(visitor);
  EventTarget::Trace(visitor);
  ExecutionContextLifecycleStateObserver::Trace(visitor);
}

}  // namespace blink
