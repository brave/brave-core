/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_TOR_CONTROL_IMPL_H_
#define BRAVE_BROWSER_TOR_TOR_CONTROL_IMPL_H_

#include <map>
#include <queue>
#include <string>

#include "brave/common/tor/tor_control.h"

#include "base/memory/scoped_refptr.h"
#include "base/observer_list.h"
#include "base/time/time.h"

namespace base { class FilePathWatcher; }
namespace base { class SequencedTaskRunner; }
namespace net { class DrainableIOBuffer; }
namespace net { class GrowableIOBuffer; }
namespace net { class TCPClientSocket; }

namespace tor {

class TorControlImpl : public TorControl {
 public:
  TorControlImpl();

  void Start(const base::FilePath& watchDirPath) override;
  void Stop() override;

  void AddObserver(TorControlObserver* observer) override;
  void RemoveObserver(TorControlObserver* observer) override;

  void Cmd1(const std::string& cmd, CmdCallback callback) override;
  void Cmd(const std::string& cmd,
           PerLineCallback perline, CmdCallback callback)
    override;

  void Subscribe(TorControlEvent event,
                 base::OnceCallback<void(bool error)> callback)
    override;
  void Unsubscribe(TorControlEvent event,
                   base::OnceCallback<void(bool error)> callback)
    override;

  void GetVersion(
      base::OnceCallback<void(
          bool error, const std::string& version)> callback)
    override;

 protected:
  friend class TorControlTest;
  FRIEND_TEST_ALL_PREFIXES(TorControlTest, ParseQuoted);
  FRIEND_TEST_ALL_PREFIXES(TorControlTest, ParseKV);

  static bool ParseKV(const std::string& string,
                      std::string& key, std::string& value);
  static bool ParseKV(const std::string& string,
                      std::string& key, std::string& value, size_t& end);
  static bool ParseQuoted(const std::string& string,
                          std::string& value, size_t& end);

 private:
  friend class base::RefCounted<TorControlImpl>;
  ~TorControlImpl() final;

  bool running_;
  SEQUENCE_CHECKER(sequence_checker_);

  scoped_refptr<base::SequencedTaskRunner> watch_task_runner_;
  SEQUENCE_CHECKER(watch_sequence_checker_);
  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;
  SEQUENCE_CHECKER(io_sequence_checker_);

  // Connection state machine.
  base::FilePath watch_dir_path_;
  std::unique_ptr<base::FilePathWatcher> watcher_;
  bool polling_;
  bool repoll_;

  std::unique_ptr<net::TCPClientSocket> socket_;

  // Write state machine.
  std::queue<std::string> writeq_;
  bool writing_;
  scoped_refptr<net::DrainableIOBuffer> writeiobuf_;

  // Read state machine.
  std::queue<std::pair<PerLineCallback, CmdCallback> > cmdq_;
  bool reading_;
  scoped_refptr<net::GrowableIOBuffer> readiobuf_;
  int read_start_;              // offset where the current line starts
  bool read_cr_;                // true if we have parsed a CR

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

  base::ObserverList<TorControlObserver> observers_;

  void StartWatching(base::FilePath watchDirPath);
  void StopWatching();
  void WatchDirChanged(const base::FilePath& path, bool error);
  void Poll();
  void Polled();
  bool EatControlCookie(std::string&, base::Time&);
  bool EatControlPort(int&, base::Time&);

  void OpenControl(int port, std::string cookie);
  void Connected(std::string cookie, int rv);
  void Authenticated(
      bool error, const std::string& status, const std::string& reply);

  void DoCmd(std::string cmd, PerLineCallback perline, CmdCallback callback);

  void GetVersionLine(
      std::string* version,
      const std::string& status, const std::string& line);
  void GetVersionDone(
      std::unique_ptr<std::string> version,
      base::OnceCallback<void(
          bool error, const std::string& version)> callback,
      bool error, const std::string& status, const std::string& reply);

  void DoSubscribe(
      TorControlEvent event, base::OnceCallback<void(bool error)> callback);
  void Subscribed(
      TorControlEvent event, base::OnceCallback<void(bool error)> callback,
      bool error, const std::string& status, const std::string& reply);
  void DoUnsubscribe(
      TorControlEvent event, base::OnceCallback<void(bool error)> callback);
  void Unsubscribed(
      TorControlEvent event, base::OnceCallback<void(bool error)> callback,
      bool error, const std::string& status, const std::string& reply);
  std::string SetEventsCmd();

  void StartWrite();
  void DoWrites();
  void WriteDoneAsync(int rv);
  void WriteDone(int rv);

  void StartRead();
  void DoReads();
  void ReadDoneAsync(int rv);
  void ReadDone(int rv);
  bool ReadLine(const std::string& line);

  void Error();

  DISALLOW_COPY_AND_ASSIGN(TorControlImpl);
};

}  // namespace tor

#endif  // BRAVE_BROWSER_TOR_TOR_CONTROL_IMPL_H_
