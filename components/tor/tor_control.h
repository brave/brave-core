/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_TOR_CONTROL_H_
#define BRAVE_COMPONENTS_TOR_TOR_CONTROL_H_

#include <map>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback_forward.h"
#include "base/gtest_prod_util.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/tor/tor_control_event.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace net {
class DrainableIOBuffer;
class GrowableIOBuffer;
class TCPClientSocket;
}  // namespace net

namespace tor {

// This class is resposible for talking to Tor executable to get status, send
// command or subsribe to events through the control channel.
// Tor Control Channel spec:
// https://gitlab.torproject.org/tpo/core/torspec/-/raw/HEAD/control-spec.txt
//
// Most of its internal implementation should be ran on IO thread so owner of
// TorControl should pass in named io task runner and also use
// base::OnTaskRunnerDeleter or calling base::SequencedTaskRunner::DeleteSoon to
// invalidate the weak ptr on the correct sequence.
// When calling API with callback, caller should use base::BindPostTask to make
// sure callback will be ran on the dedicated thread.
class TorControl {
 public:
  using PerLineCallback =
      base::RepeatingCallback<void(const std::string& status,
                                   const std::string& reply)>;
  using CmdCallback = base::OnceCallback<
      void(bool error, const std::string& status, const std::string& reply)>;

  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual void OnTorControlReady() = 0;
    virtual void OnTorControlClosed(bool was_running) = 0;

    virtual void OnTorEvent(
        TorControlEvent,
        const std::string& initial,
        const std::map<std::string, std::string>& extra) = 0;

    // Debugging options.
    virtual void OnTorRawCmd(const std::string& cmd) {}
    virtual void OnTorRawAsync(const std::string& status,
                               const std::string& line) {}
    virtual void OnTorRawMid(const std::string& status,
                             const std::string& line) {}
    virtual void OnTorRawEnd(const std::string& status,
                             const std::string& line) {}

    // Returns a WeakPtr to the implementation instance.
    virtual base::WeakPtr<Delegate> AsWeakPtr() = 0;
  };

  TorControl(base::WeakPtr<TorControl::Delegate> delegate,
             scoped_refptr<base::SequencedTaskRunner> task_runner);
  virtual ~TorControl();

  void Start(std::vector<uint8_t> cookie, int port);
  void Stop();

  void Subscribe(TorControlEvent event,
                 base::OnceCallback<void(bool error)> callback);
  void Unsubscribe(TorControlEvent event,
                   base::OnceCallback<void(bool error)> callback);

  void GetVersion(
      base::OnceCallback<void(bool error, const std::string& version)>
          callback);
  void GetSOCKSListeners(
      base::OnceCallback<void(bool error,
                              const std::vector<std::string>& listeners)>
          callback);
  void GetCircuitEstablished(
      base::OnceCallback<void(bool error, bool established)> callback);

  void SetupPluggableTransport(const base::FilePath& snowflake,
                               const base::FilePath& obfs4,
                               base::OnceCallback<void(bool error)> callback);
  void SetupBridges(const std::vector<std::string>& bridges,
                    base::OnceCallback<void(bool error)> callback);

 protected:
  friend class TorControlTest;
  FRIEND_TEST_ALL_PREFIXES(TorControlTest, ParseQuoted);
  FRIEND_TEST_ALL_PREFIXES(TorControlTest, ParseKV);
  FRIEND_TEST_ALL_PREFIXES(TorControlTest, ReadDone);
  FRIEND_TEST_ALL_PREFIXES(TorControlTest, ReadLine);
  FRIEND_TEST_ALL_PREFIXES(TorControlTest, GetCircuitEstablishedDone);

  static bool ParseKV(const std::string& string,
                      std::string* key,
                      std::string* value);
  static bool ParseKV(const std::string& string,
                      std::string* key,
                      std::string* value,
                      size_t* end);
  static bool ParseQuoted(const std::string& string,
                          std::string* value,
                          size_t* end);

 private:
  void OpenControl(int port, std::vector<uint8_t> cookie);
  void StopOnTaskRunner();
  void Connected(std::vector<uint8_t> cookie, int rv);
  void Authenticated(bool error,
                     const std::string& status,
                     const std::string& reply);

  void DoCmd(std::string cmd, PerLineCallback perline, CmdCallback callback);

  void GetVersionLine(std::string* version,
                      const std::string& status,
                      const std::string& line);
  void GetVersionDone(
      std::unique_ptr<std::string> version,
      base::OnceCallback<void(bool error, const std::string& version)> callback,
      bool error,
      const std::string& status,
      const std::string& reply);
  void GetSOCKSListenersLine(std::vector<std::string>* listeners,
                             const std::string& status,
                             const std::string& reply);
  void GetSOCKSListenersDone(
      std::unique_ptr<std::vector<std::string>> listeners,
      base::OnceCallback<
          void(bool error, const std::vector<std::string>& listeners)> callback,
      bool error,
      const std::string& status,
      const std::string& reply);
  void GetCircuitEstablishedLine(std::string* established,
                                 const std::string& status,
                                 const std::string& reply);
  void GetCircuitEstablishedDone(
      std::unique_ptr<std::string> established,
      base::OnceCallback<void(bool error, bool established)> callback,
      bool error,
      const std::string& status,
      const std::string& reply);

  void DoSubscribe(TorControlEvent event,
                   base::OnceCallback<void(bool error)> callback);
  void Subscribed(TorControlEvent event,
                  base::OnceCallback<void(bool error)> callback,
                  bool error,
                  const std::string& status,
                  const std::string& reply);
  void DoUnsubscribe(TorControlEvent event,
                     base::OnceCallback<void(bool error)> callback);
  void Unsubscribed(TorControlEvent event,
                    base::OnceCallback<void(bool error)> callback,
                    bool error,
                    const std::string& status,
                    const std::string& reply);
  std::string SetEventsCmd();

  void OnPluggableTransportsConfigured(
      base::OnceCallback<void(bool error)> callback,
      bool error,
      const std::string& status,
      const std::string& reply);
  void OnBrigdesConfigured(base::OnceCallback<void(bool error)> callback,
                           bool error,
                           const std::string& status,
                           const std::string& reply);

  // Notify delegate on UI thread
  void NotifyTorControlReady();
  void NotifyTorControlClosed();

  void NotifyTorEvent(TorControlEvent,
                      const std::string& initial,
                      const std::map<std::string, std::string>& extra);
  void NotifyTorRawCmd(const std::string& cmd);
  void NotifyTorRawAsync(const std::string& status, const std::string& line);
  void NotifyTorRawMid(const std::string& status, const std::string& line);
  void NotifyTorRawEnd(const std::string& status, const std::string& line);

  void StartWrite();
  void DoWrites();
  void WriteDoneAsync(int rv);
  void WriteDone(int rv);

  void StartRead();
  void DoReads();
  void ReadDoneAsync(int rv);
  void ReadDone(int rv);
  bool ReadLine(std::string_view line);

  void Error();

  TorControl(const TorControl&) = delete;
  TorControl& operator=(const TorControl&) = delete;

  bool running_;
  scoped_refptr<base::SequencedTaskRunner> owner_task_runner_;
  SEQUENCE_CHECKER(owner_sequence_checker_);

  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;
  SEQUENCE_CHECKER(io_sequence_checker_);

  std::unique_ptr<net::TCPClientSocket> socket_;

  // Write state machine.
  std::queue<std::string> writeq_;
  bool writing_;
  scoped_refptr<net::DrainableIOBuffer> writeiobuf_;

  // Read state machine.
  std::queue<std::pair<PerLineCallback, CmdCallback>> cmdq_;
  bool reading_;
  scoped_refptr<net::GrowableIOBuffer> readiobuf_;
  int read_start_;  // offset where the current line starts
  bool read_cr_;    // true if we have parsed a CR

  // Asynchronous command response callback state machine.
  std::map<TorControlEvent, size_t> async_events_;
  struct Async {
    Async();
    ~Async();
    TorControlEvent event;
    std::string initial;
    std::map<std::string, std::string> extra;
    bool skip;
  };
  std::unique_ptr<Async> async_;

  base::WeakPtr<TorControl::Delegate> delegate_;

  base::WeakPtrFactory<TorControl> weak_ptr_factory_{this};
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_TOR_CONTROL_H_
