/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_TOR_CONTROL_H_
#define BRAVE_BROWSER_TOR_TOR_CONTROL_H_

#include <string>

#include "brave/common/tor/tor_control_event.h"
#include "brave/common/tor/tor_control_observer.h"

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"

namespace tor {

class TorControl : public base::RefCounted<TorControl> {
 public:
  using PerLineCallback =
    base::RepeatingCallback<
        void(const std::string& status, const std::string& reply)>;
  using CmdCallback =
    base::OnceCallback<
        void(bool error, const std::string& status, const std::string& reply)>;

  static scoped_refptr<TorControl> Create();

  virtual void Start(const base::FilePath& watchDirPath) = 0;
  virtual void Stop() = 0;

  virtual void AddObserver(TorControlObserver* observer) = 0;
  virtual void RemoveObserver(TorControlObserver* observer) = 0;

  virtual void Cmd1(const std::string& cmd, CmdCallback callback) = 0;
  virtual void Cmd(const std::string& cmd,
                   PerLineCallback perline, CmdCallback callback) = 0;

  virtual void Subscribe(TorControlEvent event,
                         base::OnceCallback<void(bool error)> callback) = 0;
  virtual void Unsubscribe(TorControlEvent event,
                           base::OnceCallback<void(bool error)> callback) = 0;

  virtual void GetVersion(
      base::OnceCallback<void(
          bool error, const std::string& version)> callback) = 0;

 protected:
  friend class base::RefCounted<TorControl>;

  TorControl() = default;
  virtual ~TorControl() = default;

  DISALLOW_COPY_AND_ASSIGN(TorControl);
};

}  // namespace tor

#endif  // BRAVE_BROWSER_TOR_TOR_CONTROL_H_
