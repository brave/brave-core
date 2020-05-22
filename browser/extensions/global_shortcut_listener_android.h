// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_GLOBAL_SHORTCUT_LISTENER_ANDROID_H_
#define CHROME_BROWSER_EXTENSIONS_GLOBAL_SHORTCUT_LISTENER_ANDROID_H_

#include "base/macros.h"
#include "chrome/browser/extensions/global_shortcut_listener.h"

namespace extensions {

// Android-specific implementation of the GlobalShortcutListener class that
// listens for global shortcuts. Handles basic keyboard intercepting and
// forwards its output to the base class for processing.
class GlobalShortcutListenerAndroid : public GlobalShortcutListener {
 public:
  GlobalShortcutListenerAndroid();
  ~GlobalShortcutListenerAndroid() override;

 private:
  // GlobalShortcutListener implementation.
  void StartListening() override;
  void StopListening() override;
  bool RegisterAcceleratorImpl(const ui::Accelerator& accelerator) override;
  void UnregisterAcceleratorImpl(const ui::Accelerator& accelerator) override;

  // Whether this object is listening for global shortcuts.
  bool is_listening_;

  DISALLOW_COPY_AND_ASSIGN(GlobalShortcutListenerAndroid);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_GLOBAL_SHORTCUT_LISTENER_ANDROID_H_
