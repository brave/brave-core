/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/tor/tor_control_impl.h"

#include "brave/common/tor/tor_control.h"
#include "brave/common/tor/tor_control_observer.h"

#include "base/bind_helpers.h"
#include "base/files/file.h"
#include "base/files/file_path_watcher.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/task/post_task.h"
#include "base/task/task_traits.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/time/time.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/socket/tcp_client_socket.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

#define RetainedRef Unretained

namespace tor {

namespace {

const net::NetworkTrafficAnnotationTag tor_control_traffic_annotation =
  net::DefineNetworkTrafficAnnotation("tor_control", R"(
    semantics {
      sender: "Private browsing with Tor"
      description: "Channel for controlling the Tor networking process."
      trigger: "Tor"
      data: "Tor control messages"
      destination: LOCAL
    }
    policy {
      cookies_allowed: NO
    }
  )");

const size_t kTorBufferSize = 4096;

constexpr base::TaskTraits kIOTaskTraits = {content::BrowserThread::IO};

constexpr base::TaskTraits kWatchTaskTraits = {
  base::ThreadPool(), base::MayBlock(), base::TaskPriority::BEST_EFFORT
};

}

scoped_refptr<TorControl> TorControl::Create() {
  return base::MakeRefCounted<TorControlImpl>();
}

// try base::SequencedTaskRunnerHandle::Get()
// try base::CreateSingleThreadTaskRunner({BrowserThread::IO})
using content::BrowserThread;
TorControlImpl::TorControlImpl()
  : running_(false),
    watch_task_runner_(base::CreateSequencedTaskRunner(kWatchTaskTraits)),
    io_task_runner_(base::CreateSingleThreadTaskRunner(kIOTaskTraits)),
    polling_(false),
    repoll_(false),
    writing_(false),
    reading_(false),
    read_start_(-1),
    read_cr_(false) {
  DETACH_FROM_SEQUENCE(watch_sequence_checker_);
  DETACH_FROM_SEQUENCE(io_sequence_checker_);
}

TorControlImpl::~TorControlImpl() = default;

void TorControlImpl::AddObserver(TorControlObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.AddObserver(observer);
}

void TorControlImpl::RemoveObserver(TorControlObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.RemoveObserver(observer);
}

// Start()
//
//      Start watching for the Tor control channel.  If we are able to
//      connect, issue TorControlObserver::OnTorControlReady to all
//      observers.
//
void TorControlImpl::Start(const base::FilePath& watchDirPath) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(!running_);

  running_ = true;
  watch_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&TorControlImpl::StartWatching,
                     base::RetainedRef(this), watchDirPath));
}

void TorControlImpl::StartWatching(base::FilePath watchDirPath) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watch_sequence_checker_);
  CHECK(watch_dir_path_.empty());
  CHECK(!watcher_);

  // Create a watcher and start watching.
  watch_dir_path_ = std::move(watchDirPath);
  watcher_ = std::make_unique<base::FilePathWatcher>();
  bool recursive = false;
  if (!watcher_->Watch(watch_dir_path_,
                       recursive,
                       base::BindRepeating(&TorControlImpl::WatchDirChanged,
                                           base::Unretained(this)))) {
    // Never mind -- destroy the watcher and stop everything else.
    LOG(ERROR) << "tor: failed to watch directory";
    watcher_.reset();
    return;
  }

  polling_ = true;
  Poll();
}

// Stop()
//
//      Stop watching for the Tor control channel, and disconnect if
//      we have already connected.
//
void TorControlImpl::Stop() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(running_);

  running_ = false;
  watch_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&TorControlImpl::StopWatching, base::RetainedRef(this)));
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&TorControlImpl::Error, base::RetainedRef(this)));
}

void TorControlImpl::StopWatching() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watch_sequence_checker_);

  repoll_ = false;
  watcher_.reset();
  watch_dir_path_.clear();
}

///////////////////////////////////////////////////////////////////////////////
// Watching for startup

// WatchDirChanged(path, error)
//
//      Something happened in the watch directory at path.  If we're
//      already polling, make sure to try again if it fails -- the tor
//      daemon may now be ready if it wasn't before.  Otherwise, start
//      polling.
//
void TorControlImpl::WatchDirChanged(const base::FilePath& path, bool error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watch_sequence_checker_);
  LOG(ERROR) << "tor: watch directory changed";

  if (polling_) {
    repoll_ = true;
  } else {
    CHECK(!repoll_);
    polling_ = true;
    Poll();
  }
}

// Poll()
//
//      Something happened in the watch directory.  See whether we
//      have a control cookie and control port to connect to, and if
//      so, start connecting.  Must be done in a separate task because
//      it does file I/O which may block.
//
void TorControlImpl::Poll() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watch_sequence_checker_);
  CHECK(polling_);

  std::string cookie;
  base::Time cookie_mtime;
  int port;
  base::Time port_mtime;

  if (!EatControlCookie(cookie, cookie_mtime))
    return Polled();
  if (!EatControlPort(port, port_mtime))
    return Polled();

  // Tor writes the control port first, then the auth cookie.  If the
  // auth cookie is _older_ than the control port, then it's certainly
  // stale.  If they are the _same age_, then probably the control
  // port is older but the file system resolution is just not enough
  // to distinguish them.
  if (cookie_mtime < port_mtime) {
    LOG(ERROR) << "tor: tossing stale cookie";
    return Polled();
  }

  // Blocking shenanigans all done; move back to the regular sequence.
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&TorControlImpl::OpenControl,
                     base::Unretained(this), port, std::move(cookie)));
}

// EatControlCookie(cookie, mtime)
//
//      Try to read the control auth cookie.  Return true and set
//      cookie and mtime if successful; return false on failure.
//
bool TorControlImpl::EatControlCookie(std::string& cookie, base::Time& mtime) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watch_sequence_checker_);
  CHECK(polling_);

  // Open the control auth cookie file.
  base::FilePath cookiepath = watch_dir_path_.Append("control_auth_cookie");
  base::File cookiefile(cookiepath,
                        base::File::FLAG_OPEN|base::File::FLAG_READ);
  if (!cookiefile.IsValid()) {
    LOG(ERROR) << "tor: failed to open control auth cookie";
    return false;
  }

  // Get the file's info, including modification time.
  base::File::Info info;
  if (!cookiefile.GetInfo(&info)) {
    LOG(ERROR) << "tor: failed to stat control auth cookie";
    return false;
  }

  // Read up to 33 octets.  We should need no more than 32, so 33 will
  // indicate the file is abnormally large.
  constexpr size_t bufsiz = 33;
  char buf[bufsiz];
  int nread = cookiefile.ReadAtCurrentPos(buf, bufsiz);
  if (nread < 0) {
    LOG(ERROR) << "tor: failed to read Tor control auth cookie";
    return false;
  }
  if (nread > 32) {
    LOG(ERROR) << "tor: control auth cookie too large";
    return false;
  }

  // Success!
  cookie.assign(buf, 0, nread);
  mtime = info.last_accessed;
  // XXX DEBUG
  LOG(ERROR) << "Control cookie " << base::HexEncode(buf, nread)
             << ", mtime " << mtime;
  return true;
}

// EatControlPort(port, mtime)
//
//      Try to read the control port number.  Return true and set
//      port and mtime if successful; return false on failure.
//
bool TorControlImpl::EatControlPort(int& port, base::Time& mtime) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watch_sequence_checker_);
  CHECK(polling_);

  // Open the control port file.
  base::FilePath portpath = watch_dir_path_.Append("controlport");
  base::File portfile(portpath, base::File::FLAG_OPEN|base::File::FLAG_READ);
  if (!portfile.IsValid()) {
    LOG(ERROR) << "tor: failed to open control port";
    return false;
  }

  // Get the file's info, including modification time.
  base::File::Info info;
  if (!portfile.GetInfo(&info)) {
    LOG(ERROR) << "tor: failed to stat control port";
    return false;
  }

  // Read up to 27 octets, the maximum we will ever need.
  const char mintmpl[] = "PORT=1.1.1.1:1\n";
  const char maxtmpl[] = "PORT=255.255.255.255:65535\n";
  char buf[strlen(maxtmpl)];
  int nread = portfile.ReadAtCurrentPos(buf, sizeof buf);
  if (nread < 0) {
    LOG(ERROR) << "tor: failed to read control port";
    return false;
  }
  if (static_cast<size_t>(nread) < strlen(mintmpl)) {
    LOG(ERROR) << "tor: control port truncated";
    return false;
  }
  CHECK(static_cast<size_t>(nread) <= sizeof buf);

  std::string text(buf, 0, nread);

  // Sanity-check the content.
  if (!base::StartsWith(text, "PORT=", base::CompareCase::SENSITIVE) ||
      !base::EndsWith(text, "\n", base::CompareCase::SENSITIVE)) {
    LOG(ERROR) << "tor: invalid control port: "
               << "`" << text << ";"; // XXX escape
    return false;
  }

  // Verify that it's localhost.
  const char expected[] = "PORT=127.0.0.1:";
  if (!base::StartsWith(text, expected, base::CompareCase::SENSITIVE)) {
    LOG(ERROR) << "tor: control port has non-local control address";
    return false;
  }

  // Parse it!
  std::string portstr(text, strlen(expected), nread - 1 - strlen(expected));
  if (!base::StringToInt(portstr, &port)) {
    LOG(ERROR) << "tor: failed to parse control port: "
               << "`" << portstr << "'"; // XXX escape
    return false;
  }
  mtime = info.last_modified;
  // XXX DEBUG
  LOG(ERROR) << "Control port " << port << ", mtime " << mtime;
  return true;
}

// Polled()
//
//      Just finished polling the watch directory and failed to
//      establish a connection.  Decide whether to go back to watching
//      and waiting or whether to poll again, if something else
//      happened on the file system while we were busy polling.
//
void TorControlImpl::Polled() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watch_sequence_checker_);
  CHECK(polling_);

  if (repoll_) {
    LOG(ERROR) << "tor: retrying control connection";
    repoll_ = false;
    watch_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&TorControlImpl::Poll, base::Unretained(this)));
  } else {
    LOG(ERROR) << "tor: control connection not yet ready";
    polling_ = false;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Opening the connection and authenticating

// OpenControl(portno, cookie)
//
//      Open a control connection on the specified port number at
//      localhost, with the specified control auth cookie.
//
void TorControlImpl::OpenControl(int portno, std::string cookie) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  // XXX DEBUG
  LOG(ERROR) << __func__ << " " << base::HexEncode(cookie.data(), cookie.size());

  net::AddressList addrlist =
    net::AddressList::CreateFromIPAddress(net::IPAddress::IPv4Localhost(),
                                          portno);
  socket_ = std::make_unique<net::TCPClientSocket>(
      addrlist, nullptr, net::NetLog::Get(), net::NetLogSource());
  int rv = socket_->Connect(
      base::BindOnce(&TorControlImpl::Connected,
                     base::Unretained(this), std::move(cookie)));
  if (rv == net::ERR_IO_PENDING)
    return;
  Connected(std::move(cookie), rv);
}

// Connected(rv, cookie)
//
//      Connection completed.  If it failed, poll again if there was
//      activity while we were busy connecting, or go back to watching
//      and waiting.  If it succeeded, start authenticating.
//
void TorControlImpl::Connected(std::string cookie, int rv) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);

  if (rv != net::OK) {
    // Connection failed but there may have been more watch directory
    // activity while we were waiting.  If so, try again; if not, go
    // back to watching and waiting.
    LOG(ERROR) << "tor: control connection failed: "
               << net::ErrorToString(rv);
    watch_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&TorControlImpl::Polled, base::Unretained(this)));
    return;
  }

  Cmd1("AUTHENTICATE " + base::HexEncode(cookie.data(), cookie.size()),
       base::BindOnce(&TorControlImpl::Authenticated,
                      base::Unretained(this)));
}

// Authenticated(error, status, reply)
//
//      Tor control AUTHENTICATE command callback.  If we failed, kill
//      the connection and start over.  If we succeeded, announce that
//      we're ready.
//
void TorControlImpl::Authenticated(bool error,
                                   const std::string& status,
                                   const std::string& reply) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  if (!error) {
    if (status != "250" || reply != "OK")
      error = true;
  }
  if (error) {
    LOG(ERROR) << "tor: control authentication failed";
    return;
  }
  LOG(ERROR) << "tor: control connection ready";
  for (auto& observer : observers_)
    observer.OnTorControlReady();
}

///////////////////////////////////////////////////////////////////////////////
// Event subscriptions

// Subscribe(event, callback)
//
//      Subscribe to event by sending SETEVENTS with it included
//      (along with all previously subscribed events).  If repeated,
//      just increment nesting depth without sending SETEVENTS.  Call
//      the callback once the subscription has been processed.
//      Subsequently, whenever the event happens, notify the
//      OnTorEvent observers.
//
void TorControlImpl::Subscribe(
    TorControlEvent event, base::OnceCallback<void(bool error)> callback) {
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&TorControlImpl::DoSubscribe,
                     base::Unretained(this), event, std::move(callback)));
}

void TorControlImpl::DoSubscribe(
    TorControlEvent event, base::OnceCallback<void(bool error)> callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  if (async_events_[event]++ != 0) {
    bool error = false;
    std::move(callback).Run(error);
    return;
  }

  async_events_[event] = 1;
  Cmd1(SetEventsCmd(),
       base::BindOnce(&TorControlImpl::Subscribed,
                      base::Unretained(this), event, std::move(callback)));
}

void TorControlImpl::Subscribed(
    TorControlEvent event, base::OnceCallback<void(bool error)> callback,
    bool error, const std::string& status, const std::string& reply) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  if (!error) {
    if (status != "250")
      error = true;
  }
  if (error) {
    if (--async_events_[event] == 0)
      async_events_.erase(event);
  }
  std::move(callback).Run(error);
}

// Unsubscribe(event, callback)
//
//      Unsubscribe to the named asynchronous event by sending
//      SETEVENTS with it excluded from all otherwise subscribed
//      events.  Caller must already be subscribed.  If used after
//      repeated Subscribe with the same event, just decrement nesting
//      depth without sending SETEVENTS.  Call the callback once the
//      unsubscription has been processed.  Subsequently, the event
//      will not trigger notification of OnTorEvent observers.
//
void TorControlImpl::Unsubscribe(
    TorControlEvent event, base::OnceCallback<void(bool error)> callback) {
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&TorControlImpl::DoUnsubscribe,
                     base::Unretained(this), event, std::move(callback)));
}

void TorControlImpl::DoUnsubscribe(
    TorControlEvent event, base::OnceCallback<void(bool error)> callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  // We had better already be subscribed.
  CHECK(async_events_[event] >= 1);
  if (--async_events_[event] != 0) {
    bool error = false;
    std::move(callback).Run(error);
    return;
  }

  CHECK(async_events_[event] == 0);
  async_events_.erase(event);
  Cmd1(SetEventsCmd(),
       base::BindOnce(&TorControlImpl::Unsubscribed,
                      base::Unretained(this), event, std::move(callback)));
}

void TorControlImpl::Unsubscribed(
    TorControlEvent event, base::OnceCallback<void(bool error)> callback,
    bool error, const std::string& status, const std::string& reply) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  CHECK(async_events_.count(event) == 0);
  if (!error) {
    if (status != "250")
      error = true;
  }
  std::move(callback).Run(error);
}

// SetEventsCmd()
//
//      Return a SETEVENTS command with our current asynchronous event
//      subscriptions.
//
std::string TorControlImpl::SetEventsCmd() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  std::ostringstream cmds;
  cmds << "SETEVENTS";
  for (const auto& entry : async_events_) {
    const auto& found = kTorControlEventByEnum.find(entry.first);
    CHECK(found != kTorControlEventByEnum.end());
    cmds << " " << (*found).second;
  }
  return cmds.str();
}

///////////////////////////////////////////////////////////////////////////////
// Sending commands

// Cmd1(cmd, callback)
//
//      Issue a Tor control command for which we only care about the
//      final line; ignore all intermediate lines.
//
void TorControlImpl::Cmd1(const std::string& cmd, CmdCallback callback) {
  Cmd(cmd,
      base::DoNothing::Repeatedly<const std::string&, const std::string&>(),
      std::move(callback));
}

// Cmd(cmd, perline, callback)
//
//      Issue a Tor control command.  Call perline for each
//      intermediate line; then call callback for the last line or on
//      error.
//
void TorControlImpl::Cmd(const std::string& cmd,
                         PerLineCallback perline,
                         CmdCallback callback) {
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&TorControlImpl::DoCmd, base::Unretained(this),
                     cmd, std::move(perline), std::move(callback)));
}

void TorControlImpl::DoCmd(std::string cmd,
                           PerLineCallback perline,
                           CmdCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  for (auto& observer : observers_)
    observer.OnTorRawCmd(cmd);
  if (!socket_ || writeq_.size() > 100 || cmdq_.size() > 100) {
    // Socket is closed, or over 100 commands pending or synchronous
    // callbacks queued -- something is probably wrong.
    bool error = true;
    std::move(callback).Run(error, "", "");
    return;
  }
  writeq_.push(cmd + "\r\n");
  cmdq_.push(
    std::make_pair<PerLineCallback, CmdCallback>(
      std::move(perline), std::move(callback)));
  if (!writing_) {
    writing_ = true;
    StartWrite();
    DoWrites();
  }
  if (!reading_) {
    reading_ = true;
    StartRead();
    DoReads();
  }
}

// GetVersion(callback)
//
//      Get the Tor version and call callback(error, version).
//
void TorControlImpl::GetVersion(
    base::OnceCallback<void(
        bool error, const std::string& version)> callback) {
  std::unique_ptr<std::string> version = std::make_unique<std::string>();
  std::string* versionp = version.get();
  Cmd("GETINFO version",
      base::BindRepeating(&TorControlImpl::GetVersionLine,
                          base::Unretained(this), versionp),
      base::BindOnce(&TorControlImpl::GetVersionDone,
                     base::Unretained(this),
                     std::move(version), std::move(callback)));
}

void TorControlImpl::GetVersionLine(std::string* version,
                                    const std::string& status,
                                    const std::string& reply) {
  const char prefix[] = "version=";
  if (status != "250" ||
      !base::StartsWith(reply, prefix, base::CompareCase::SENSITIVE) ||
      !version->empty()) {
    LOG(ERROR) << "tor: unexpected `GETINFO version' reply";
    return;
  }
  *version = reply.substr(strlen(prefix));
}

void TorControlImpl::GetVersionDone(
    std::unique_ptr<std::string> version,
    base::OnceCallback<void(bool error, const std::string& version)> callback,
    bool error, const std::string& status, const std::string& reply) {
  if (error ||
      status != "250" ||
      reply != "OK" ||
      version->empty()) {
    std::move(callback).Run(true, "");
    return;
  }
  std::move(callback).Run(false, *version);
}

///////////////////////////////////////////////////////////////////////////////
// Writing state machine

// StartWrite()
//
//      Pick a write off the queue and start an I/O buffer for it.
//
//      Caller must ensure writing_ is true.
//
void TorControlImpl::StartWrite() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  CHECK(writing_);
  CHECK(!writeq_.empty());
  CHECK(!cmdq_.empty());
  auto buf = base::MakeRefCounted<net::StringIOBuffer>(writeq_.front());
  writeiobuf_ = base::MakeRefCounted<net::DrainableIOBuffer>(buf, buf->size());
  writeq_.pop();
}

// XXX DEBUG
static std::string escapify(const char *buf, int len) {
  std::ostringstream s;
  for (int i = 0; i < len; i++) {
    unsigned char ch = static_cast<unsigned char>(buf[i]);
    if (::isprint(ch)) {
      s << buf[i];
      continue;
    }
    switch (ch) {
      case '\f': s << "\\f"; break;
      case '\n': s << "\\n"; break;
      case '\r': s << "\\r"; break;
      case '\t': s << "\\t"; break;
      default:
        const char hex[] = "0123456789abcdef";
        s << "\\x";
        s << hex[(ch >> 4) & 0xf];
        s << hex[(ch >> 0) & 0xf];
        break;
    }
  }
  return s.str();
}

// DoWrites()
//
//      Issue writes from writeiobuf_, and arrange to issue the rest
//      of the writes in the queue when done.
//
//      Caller must ensure writing_ is true and writeiobuf_ is
//      initialized.
//
void TorControlImpl::DoWrites() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  CHECK(writing_);
  CHECK(writeiobuf_);
  int rv;
  while ((rv = socket_->Write(writeiobuf_.get(), writeiobuf_->size(),
                              base::BindOnce(&TorControlImpl::WriteDoneAsync,
                                             base::Unretained(this)),
                              tor_control_traffic_annotation))
      != net::ERR_IO_PENDING) {
    WriteDone(rv);
    if (!writing_)
      break;
  }
}

// WriteDoneAsync(rv)
//
//      Asynchronous callback for write completion.  Defer to
//      WriteDone() and then start up DoWrites() again if need be.
//
//      Caller must ensure writing_ is true and writeiobuf_ is
//      initialized.
//
void TorControlImpl::WriteDoneAsync(int rv) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  CHECK(writing_);
  CHECK(writeiobuf_);
  WriteDone(rv);
  if (writing_)
    DoWrites();
}

// WriteDone(rv)
//
//      Handle write completion.  Advance the write buffer, reissue it
//      if not complete, or if complete pick the next write off the
//      queue and issue it.  If there's no more work to do, disable
//      writing_.
//
//      Caller must ensure writing_ is true and writeiobuf_ is
//      initialized.
//
void TorControlImpl::WriteDone(int rv) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  CHECK(writing_);
  CHECK(writeiobuf_);
  // Bail if error.
  if (rv < 0) {
    LOG(ERROR) << "tor: control write error: " << net::ErrorToString(rv);
    Error();
    return;
  }
  writeiobuf_->DidConsume(rv);
  // If there's nothing more in the buffer, try to get another off the queue.
  if (!writeiobuf_->BytesRemaining()) {
    // No need to hang on to the I/O buffer any longer.
    writeiobuf_.reset();
    // If there's nothing more in the queue, we're done.
    if (writeq_.empty()) {
      writing_ = false;
      return;
    }
    // More in the queue.  Start a fresh write.
    StartWrite();
  }
}

///////////////////////////////////////////////////////////////////////////////
// Reading state machine

// StartRead()
//
//      Create an I/O buffer to read command responses into.
//
//      Caller must ensure reading_ is true and that there are
//      synchronous command callbacks or asynchronous event
//      registrations.
//
void TorControlImpl::StartRead() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  CHECK(reading_);
  CHECK(!cmdq_.empty() || !async_events_.empty());
  readiobuf_ = base::MakeRefCounted<net::GrowableIOBuffer>();
  readiobuf_->SetCapacity(kTorBufferSize);
  read_start_ = 0;
  CHECK(readiobuf_->RemainingCapacity());
}

// DoReads()
//
//      Issue reads into readiobuf_ and process them.
//
//      Caller must ensure reading_ is true and readiobuf_ is
//      initialized.
//
void TorControlImpl::DoReads() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  CHECK(reading_);
  CHECK(readiobuf_);
  int rv;
  CHECK(readiobuf_->RemainingCapacity());
  while ((rv = socket_->Read(readiobuf_.get(), readiobuf_->RemainingCapacity(),
                             base::BindOnce(&TorControlImpl::ReadDoneAsync,
                                            base::Unretained(this))))
      != net::ERR_IO_PENDING) {
    ReadDone(rv);
    if (!reading_)
      break;
    CHECK(readiobuf_->RemainingCapacity());
  }
}

// ReadDoneAsync(rv)
//
//      Asynchronous callback for read completion.  Defer to
//      ReadDone() and then start up DoReads() again if need be.
//
//      Caller must ensure reading_ is true and readiobuf_ is
//      initialized.
//
void TorControlImpl::ReadDoneAsync(int rv) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  CHECK(reading_);
  CHECK(readiobuf_);
  ReadDone(rv);
  if (reading_) {
    CHECK(readiobuf_->RemainingCapacity());
    DoReads();
  }
}

// ReadDone()
//
//      A read into readiobuf_ just completed.  Process it.  If
//      there's no more reads to do, disable reading_.
//
//      Caller must ensure reading_ is true and readiobuf_ is
//      initialized.
//
void TorControlImpl::ReadDone(int rv) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  CHECK(reading_);
  CHECK(readiobuf_);
  if (rv < 0) {
    LOG(ERROR) << "tor: control read error: " << net::ErrorToString(rv);
    Error();
    return;
  }
  if (rv == 0) {
    LOG(ERROR) << "tor: control closed prematurely";
    Error();
    return;
  }
  const char *data = readiobuf_->data();
  for (int i = 0; i < rv; i++) {
    if (!read_cr_) {
      // No CR yet.  Accept CR or non-LF; reject LF.
      if (data[i] == 0x0d) {            // CR
        read_cr_ = true;
      } else if (data[i] == 0x0a) {     // LF
        LOG(ERROR) << "tor: stray line feed";
        Error();
        return;
      } else {
        // Anything else: Just accept it and move on.
      }
    } else {
      // CR seen.  Accept LF; reject all else.
      if (data[i] == 0x0a) {            // LF
        // CRLF seen, so we must have i >= 2.  Emit a line and advance
        // to the next one, unless anything went wrong with the line.
        assert(i >= 1);
        std::string line(readiobuf_->StartOfBuffer() + read_start_,
                         readiobuf_->offset() + i - 1 - read_start_);
        read_start_ = readiobuf_->offset() + i + 1;
        read_cr_ = false;
        if (!ReadLine(line)) {
          reading_ = false;
          return;
        }
      } else {
        // CR seen, but not LF.  Bad.
        LOG(ERROR) << "tor: stray carriage return";
        Error();
        return;
      }
    }
  }

  // If we've walked up to the end of the buffer, try shifting it to
  // the beginning to make room; if there's no more room, fail --
  // lines shouldn't be this long.
  CHECK(rv <= readiobuf_->RemainingCapacity());
  if (readiobuf_->RemainingCapacity() == rv) {
    if (read_start_ == 0) {
      // Line is too long.
      LOG(ERROR) << "tor: control line too long";
      Error();
      return;
    }
    memmove(readiobuf_->StartOfBuffer(),
            readiobuf_->StartOfBuffer() + read_start_,
            readiobuf_->offset() - read_start_ + rv);
    readiobuf_->set_offset(readiobuf_->offset() - read_start_ + rv);
    read_start_ = 0;
  } else {
    // Otherwise, just advance the offset by the size of this input.
    readiobuf_->set_offset(readiobuf_->offset() + rv);
  }
  CHECK(readiobuf_->RemainingCapacity());

  // If we've processed every byte in the input so far, and there's no
  // more command callbacks queued or asynchronous events registered,
  // stop.
  if (read_start_ == readiobuf_->offset() &&
      cmdq_.empty() &&
      async_events_.empty()) {
    reading_ = false;
    readiobuf_.reset();
    read_start_ = 0;
    read_cr_ = false;
    return;
  }
}

// ReadLine(line)
//
//      We have read a line of input; process it.  Return true on
//      success, false on error.
//
bool TorControlImpl::ReadLine(const std::string& line) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);

  if (line.size() < 4) {
    // Line is too short.
    LOG(ERROR) << "tor: control line too short";
    Error();
    return false;
  }

  // Parse out the line into status, position in reply stream, and
  // content: `xyzP...' where xyz are digits and P is `-' for an
  // intermediate reply and ` ' for a final reply.
  //
  // TODO(riastradh): parse or check syntax of status
  std::string status(line, 0, 3);
  char pos = line[3];
  std::string reply(line, 4);

  // Determine whether it is an asynchronous reply, status 6yz.
  if (status[0] == '6') {
    // Notify observers of the raw reply.
    for (auto& observer : observers_)
      observer.OnTorRawAsync(status, reply);

    // Is this a new async reply?
    if (!async_) {
      // Parse the keyword and the initial line.
      const size_t sp = reply.find(' ');
      std::string event_name, initial;
      if (sp == std::string::npos) {
        event_name = reply;
      } else {
        event_name = reply.substr(0, sp);
        initial = reply.substr(sp + 1);
      }

      // Discriminate on the position of the reply.
      switch (pos) {
        case ' ': {
          // Single-line async reply.

          // Bail if we don't recognize the event name.
          const auto& found = kTorControlEventByName.find(event_name);
          if (found == kTorControlEventByName.end()) {
            LOG(ERROR) << "tor: unknown event: " << event_name; // XXX escape
            return false;
          }
          const TorControlEvent event = (*found).second;

          // Ignore if we don't think we're subscribed to this.
          if (!async_events_.count(event)) {
            LOG(ERROR) << "tor: spurious event: " << event_name;
            return true;
          }

          // Notify the observers of the parsed reply.  No extra
          // because there were no intermediate reply lines.
          for (auto& observer : observers_)
            observer.OnTorEvent(event, initial, {});

          return true;
        }
        case '-': {
          // Start of a multi-line async reply.

          // Start a fresh async reply state.  Parse the rest, but
          // skip it, if we don't recognize the event.
          const auto& found = kTorControlEventByName.find(event_name);
          const TorControlEvent event = (found == kTorControlEventByName.end()
              ? TorControlEvent::INVALID
              : (*found).second);
          async_ = std::make_unique<Async>();
          async_->event = event;
          async_->initial = initial;
          async_->skip = (event == TorControlEvent::INVALID);
          return true;
        }
      }
    } else {
      // We have an async reply ongoing.  Discriminate on the position
      // of the reply.
      switch (pos) {
        case '-': {
          // Continuation of an async reply.  Add to it, unless we're
          // skipping it.
          if (async_->skip)
            return true;
          // If we're not longer subscribed, forget about it.
          if (async_events_.count(async_->event) == 0) {
            async_->skip = true;
            async_->event = TorControlEvent::INVALID;
            async_->initial.clear();
            async_->extra.clear();
            return true;
          }
          std::string key, value;
          if (!ParseKV(reply, key, value)) {
            LOG(ERROR) << "tor: invalid async continuation line";
            Error();
            return false;
          }
          if (async_->extra.count(key)) {
            LOG(ERROR) << "tor: duplicate key in async continuation line";
            Error();
            return false;
          }
          async_->extra[key] = value;
          return true;
        }
        case ' ': {
          // End of an async reply.  Parse it and finish it, unless
          // we're skipping.
          if (!async_->skip) {
            std::string key, value;
            if (!ParseKV(reply, key, value)) {
              LOG(ERROR) << "tor: invalid async event";
              Error();
              return false;
            }
            if (async_->extra.count(key)) {
              LOG(ERROR) << "tor: duplicate key in async event";
              Error();
              return false;
            }
            async_->extra[key] = value;

            // If we're still subscribed, notify the observers of the
            // parsed reply.
            if (async_events_.count(async_->event)) {
              for (auto& observer : observers_)
                observer.OnTorEvent(
                    async_->event, async_->initial, async_->extra);
            }
          }
          async_.reset();
          return true;
        }
      }
    }
  } else {
    // Synchronous reply.  Return it to the next command callback in
    // the queue.
    switch (pos) {
      case '-':
        for (auto& observer : observers_)
          observer.OnTorRawMid(status, reply);
        if (!cmdq_.empty()) {
          PerLineCallback& perline = cmdq_.front().first;
          perline.Run(status, reply);
        }
        return true;
      case '+':
        LOG(ERROR) << "tor: NYI: control data reply";
        // XXX Just ignore it for now.
        return true;
      case ' ':
        for (auto& observer : observers_)
          observer.OnTorRawEnd(status, reply);
        if (!cmdq_.empty()) {
          CmdCallback& callback = cmdq_.front().second;
          bool error = false;
          std::move(callback).Run(error, status, reply);
          cmdq_.pop();
        }
        return true;
    }
  }

  // Not reached if the line is well-formed.
  LOG(ERROR) << "tor: malformed control line: "
             << escapify(line.data(), line.size());
  Error();
  return false;
}

TorControlImpl::Async::Async() = default;
TorControlImpl::Async::~Async() = default;

// Error()
//
//      Clear read and write state and disconnect.
//
void TorControlImpl::Error() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);

  LOG(ERROR) << "tor: closing control on " << (running_ ? "request" : "error");

  // Invoke all callbacks with errors and clear read state.
  while (!cmdq_.empty()) {
    CmdCallback& callback = cmdq_.front().second;
    bool error = true;
    std::move(callback).Run(error, "", "");
    cmdq_.pop();
  }
  reading_ = false;
  readiobuf_.reset();
  read_start_ = -1;
  read_cr_ = false;

  // Clear write state.
  writeq_ = {};
  writing_ = false;
  writeiobuf_.reset();

  // Clear the socket.
  socket_.reset();

  // If we're still running, try watching again to start over.
  //
  // XXX Rate limit in case of flapping?
  if (running_) {
    watch_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&TorControlImpl::Poll, base::Unretained(this)));
  }
}

// ParseKV(string, key, value)
//
//      Parse KEY=VALUE notation from string into key and value,
//      following the Tor control spec notation.  Return true on
//      success, false on failure.
//
// static
bool TorControlImpl::ParseKV(
    const std::string& string,
    std::string& key, std::string& value) {
  size_t end;
  return ParseKV(string, key, value, end) && end == string.size();
}

// ParseKV(string, key, value, end)
//
//      Parse KEY=VALUE notation from string into key and value,
//      following the Tor control spec notation, and set end to the
//      number of octets consumed.  Return true on success, false on
//      failure.
//
// static
bool TorControlImpl::ParseKV(
    const std::string& string,
    std::string& key, std::string& value, size_t& end) {
  // Search for `=' -- it had better be there.
  size_t eq = string.find("=");
  if (eq == std::string::npos)
    return false;
  size_t vstart = eq + 1;

  // If we're at the end of the string, value is empt.
  if (vstart == string.size()) {
    key = string.substr(0, eq);
    value = "";
    end = string.size();
    return true;
  }

  // Check whether it's quoted.
  if (string[vstart] != '"') {
    // Not quoted.  Check for a delimiter.
    size_t i, vend = string.size();
    if ((i = string.find(" ", vstart)) != std::string::npos) {
      // Delimited.  Stop at the delimiter, and consume it.
      vend = i;
      end = vend + 1;
    } else {
      // Not delimited.  Stop at the end of string.
      end = vend;
    }

    // Check for internal quotes; they are forbidden.
    if ((i = string.find("\"", vstart)) != std::string::npos)
      return false;

    // Extract the key and value and we're done.
    key = string.substr(0, eq);
    value = string.substr(vstart, vend - vstart);
    return true;
  }

  // Quoted string.  Parse it, and consume trailing spaces.
  if (!ParseQuoted(string.substr(eq + 1), value, end))
    return false;
  key = string.substr(0, eq);
  end += eq + 1;
  while (end < string.size() && string[end] == ' ')
    end++;
  return true;
}

// ParseQuoted(string, value, end)
//
//      Parse a quoted string starting _after_ the initial `"'.  Set
//      value to the unquoted (and unescaped) content and end to the
//      position _after_ the final `"' and return true on success, or
//      return false on failure.
//
// static
bool TorControlImpl::ParseQuoted(const std::string& string,
                                 std::string& value, size_t& end) {
  enum {
    REJECT,
    ACCEPT,
    START,
    BODY,
    BACKSLASH,
    OCTAL1,
    OCTAL2,
  } S = START;
  std::string buf(string.size(), '\0');
  size_t i, pos = 0;
  unsigned octal;

  for (i = 0; i < string.size(); i++) {
    char ch = string[i];

    // Do a state transition for the character.  Written to make the
    // structure of the state machine clear.
    switch (S) {
      default:
      case REJECT:
      case ACCEPT:
        CHECK(0);
        S = REJECT;
        break;
      case START:
        S = (ch == '"' ? BODY : REJECT);
        break;
      case BODY:
        switch (ch) {
          case '\\':    S = BACKSLASH; break;
          case '"':     S = ACCEPT; break;
          default:      buf[pos++] = ch; S = BODY; break;
        }
        break;
      case BACKSLASH:
        switch (ch) {
          case '0': case '1': case '2': case '3':
          case '4': case '5': case '6': case '7':
            octal = (ch - '0') << 6;
            S = OCTAL1;
            break;
          case 'n':     buf[pos++] = '\n'; S = BODY; break;
          case 'r':     buf[pos++] = '\r'; S = BODY; break;
          case 't':     buf[pos++] = '\t'; S = BODY; break;
          case '\\': case '"': case '\'':
                        buf[pos++] = ch; S = BODY; break;
          default:      S = REJECT; break;
        }
        break;
      case OCTAL1:
        switch (ch) {
          case '0': case '1': case '2': case '3':
          case '4': case '5': case '6': case '7':
            octal |= (ch - '0') << 3;
            S = OCTAL2;
            break;
          default:
            S = REJECT;
            break;
        }
        break;
      case OCTAL2:
        switch (ch) {
          case '0': case '1': case '2': case '3':
          case '4': case '5': case '6': case '7':
            octal |= (ch - '0');
            buf[pos++] = octal;
            S = BODY;
            break;
          default:
            S = REJECT;
            break;
        }
        break;
    }

    // Handle reject or accept.
    switch (S) {
      case REJECT:
        return false;
      case ACCEPT:
        value = buf.substr(0, pos);
        end = i + 1;
        return true;
      default:
        break;
    }
  }

  // Consumed the whole string without accepting it.  Reject!
  return false;
}

}  // namespace tor
