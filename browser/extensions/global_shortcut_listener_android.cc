// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/extensions/global_shortcut_listener_android.h"

#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace extensions {

// static
GlobalShortcutListener* GlobalShortcutListener::GetInstance() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  static GlobalShortcutListenerAndroid* instance =
      new GlobalShortcutListenerAndroid();
  return instance;
}

GlobalShortcutListenerAndroid::GlobalShortcutListenerAndroid()
    : is_listening_(false) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  // TODO(implementor): Remove this.
  LOG(ERROR) << "GlobalShortcutListenerAndroid object created";
}

GlobalShortcutListenerAndroid::~GlobalShortcutListenerAndroid() {
  if (is_listening_)
    StopListening();
}

void GlobalShortcutListenerAndroid::StartListening() {
  DCHECK(!is_listening_);  // Don't start twice.
  NOTIMPLEMENTED();
  is_listening_ = true;
}

void GlobalShortcutListenerAndroid::StopListening() {
  DCHECK(is_listening_);  // No point if we are not already listening.
  NOTIMPLEMENTED();
  is_listening_ = false;
}

bool GlobalShortcutListenerAndroid::RegisterAcceleratorImpl(
    const ui::Accelerator& accelerator) {
  NOTIMPLEMENTED();
  // To implement:
  // 1) Convert modifiers to platform specific modifiers.
  // 2) Register for the hotkey.
  // 3) If not successful, return false.
  // 4) Else, return true.

  return false;
}

void GlobalShortcutListenerAndroid::UnregisterAcceleratorImpl(
    const ui::Accelerator& accelerator) {
  NOTIMPLEMENTED();
  // To implement: Unregister for the hotkey.
}

}  // namespace extensions
