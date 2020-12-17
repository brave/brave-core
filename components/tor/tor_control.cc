/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_control.h"

#include "base/callback_helpers.h"
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

// Tor Control Channel spec:
// https://gitweb.torproject.org/torspec.git/plain/control-spec.txt

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
#if defined(OS_WIN)
constexpr char kControlPortMinTmpl[] = "PORT=1.1.1.1:1\r\n";
constexpr char kControlPortMaxTmpl[] = "PORT=255.255.255.255:65535\r\n";
constexpr char kLineBreak[] = "\r\n";
#else
constexpr char kControlPortMinTmpl[] = "PORT=1.1.1.1:1\n";
constexpr char kControlPortMaxTmpl[] = "PORT=255.255.255.255:65535\n";
constexpr char kLineBreak[] = "\n";
#endif
constexpr char kControlAuthCookieName[] = "control_auth_cookie";
constexpr char kControlPortName[] = "controlport";
constexpr char kTorPidName[] = "tor.pid";

constexpr char kGetVersionCmd[] = "GETINFO version";
constexpr char kGetVersionReply[] = "version=";
constexpr char kGetSOCKSListenersCmd[] = "GETINFO net/listeners/socks";
constexpr char kGetSOCKSListenersReply[] = "net/listeners/socks=";

constexpr base::TaskTraits kWatchTaskTraits = {
    base::ThreadPool(), base::MayBlock(), base::TaskPriority::BEST_EFFORT};

static std::string escapify(const char* buf, int len) {
  std::ostringstream s;
  for (int i = 0; i < len; i++) {
    unsigned char ch = static_cast<unsigned char>(buf[i]);
    if (::isprint(ch)) {
      s << buf[i];
      continue;
    }
    switch (ch) {
      case '\f':
        s << "\\f";
        break;
      case '\n':
        s << "\\n";
        break;
      case '\r':
        s << "\\r";
        break;
      case '\t':
        s << "\\t";
        break;
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

}  // namespace

// static
std::unique_ptr<TorControl> TorControl::Create(TorControl::Delegate* delegate) {
  return std::make_unique<TorControl>(delegate);
}

TorControl::TorControl(TorControl::Delegate* delegate)
    : running_(false),
      watch_task_runner_(base::CreateSequencedTaskRunner(kWatchTaskTraits)),
      io_task_runner_(content::GetIOThreadTaskRunner({})),
      polling_(false),
      repoll_(false),
      writing_(false),
      reading_(false),
      read_start_(-1),
      read_cr_(false),
      delegate_(delegate) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DETACH_FROM_SEQUENCE(watch_sequence_checker_);
  DETACH_FROM_SEQUENCE(io_sequence_checker_);
}

TorControl::~TorControl() = default;

void TorControl::PreStartCheck(const base::FilePath& watchDirPath,
                               base::OnceClosure check_complete) {
  watch_dir_path_ = std::move(watchDirPath);
  watch_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&TorControl::CheckingOldTorProcess, base::Unretained(this),
                     std::move(check_complete)));
}

// Start()
//
//      Start watching for the Tor control channel.  If we are able to
//      connect, issue OnTorControlReady to delegate.
//
void TorControl::Start() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!watch_dir_path_.empty());
  DCHECK(!running_);

  running_ = true;
  watch_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&TorControl::StartWatching, base::Unretained(this)));
}

void TorControl::StartWatching() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watch_sequence_checker_);
  DCHECK(!watcher_);

  // Create a watcher and start watching.
  watcher_ = std::make_unique<base::FilePathWatcher>();

  if (!watcher_->Watch(watch_dir_path_,
                       base::FilePathWatcher::Type::kNonRecursive,
                       base::BindRepeating(&TorControl::WatchDirChanged,
                                           base::Unretained(this)))) {
    // Never mind -- destroy the watcher and stop everything else.
    VLOG(0) << "tor: failed to watch directory";
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
void TorControl::Stop() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!running_)
    return;

  running_ = false;
  async_events_.clear();
  watch_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&TorControl::StopWatching, base::Unretained(this)));
  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&TorControl::Error, base::Unretained(this)));
}

void TorControl::StopWatching() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watch_sequence_checker_);

  repoll_ = false;
  watcher_.reset();
  watch_dir_path_.clear();
}

void TorControl::CheckingOldTorProcess(base::OnceClosure callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watch_sequence_checker_);
  base::ProcessId id;
  if (EatOldPid(&id))
    NotifyTorCleanupNeeded(std::move(id));
  content::GetUIThreadTaskRunner({})->PostTask(FROM_HERE, std::move(callback));
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
void TorControl::WatchDirChanged(const base::FilePath& path, bool error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watch_sequence_checker_);
  VLOG(2) << "tor: watch directory changed";

  if (polling_) {
    repoll_ = true;
  } else {
    DCHECK(!repoll_);
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
void TorControl::Poll() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watch_sequence_checker_);
  DCHECK(polling_);

  std::vector<uint8_t> cookie;
  base::Time cookie_mtime;
  int port;
  base::Time port_mtime;

  if (!EatControlCookie(cookie, cookie_mtime))
    return PollDone();
  if (!EatControlPort(port, port_mtime))
    return PollDone();

  // Tor writes the control port first, then the auth cookie.  If the
  // auth cookie is _older_ than the control port, then it's certainly
  // stale.  If they are the _same age_, then probably the control
  // port is older but the file system resolution is just not enough
  // to distinguish them.
  if (cookie_mtime < port_mtime) {
    VLOG(0) << "tor: tossing stale cookie";
    return PollDone();
  }

  // Blocking shenanigans all done; move back to the regular sequence.
  io_task_runner_->PostTask(FROM_HERE, base::BindOnce(&TorControl::OpenControl,
                                                      base::Unretained(this),
                                                      port, std::move(cookie)));
}

// EatControlCookie(cookie, mtime)
//
//      Try to read the control auth cookie.  Return true and set
//      cookie and mtime if successful; return false on failure.
//
bool TorControl::EatControlCookie(std::vector<uint8_t>& cookie,
                                  base::Time& mtime) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watch_sequence_checker_);
  DCHECK(polling_);

  // Open the control auth cookie file.
  base::FilePath cookiepath =
      watch_dir_path_.AppendASCII(kControlAuthCookieName);
  base::File cookiefile(cookiepath,
                        base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!cookiefile.IsValid()) {
    VLOG(0) << "tor: failed to open control auth cookie";
    return false;
  }

  // Get the file's info, including modification time.
  base::File::Info info;
  if (!cookiefile.GetInfo(&info)) {
    VLOG(0) << "tor: failed to stat control auth cookie";
    return false;
  }

  // Read up to 33 octets.  We should need no more than 32, so 33 will
  // indicate the file is abnormally large.
  constexpr size_t kBufSiz = 33;
  char buf[kBufSiz];
  int nread = cookiefile.ReadAtCurrentPos(buf, kBufSiz);
  if (nread < 0) {
    VLOG(0) << "tor: failed to read Tor control auth cookie";
    return false;
  }
  if (nread > 32) {
    VLOG(0) << "tor: control auth cookie too large";
    return false;
  }

  // Success!
  cookie.assign(buf, buf + nread);
  mtime = info.last_accessed;
  VLOG(3) << "Control cookie " << base::HexEncode(buf, nread) << ", mtime "
          << mtime;
  return true;
}

// EatControlPort(port, mtime)
//
//      Try to read the control port number.  Return true and set
//      port and mtime if successful; return false on failure.
//
bool TorControl::EatControlPort(int& port, base::Time& mtime) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watch_sequence_checker_);
  DCHECK(polling_);

  // Open the control port file.
  base::FilePath portpath = watch_dir_path_.AppendASCII(kControlPortName);
  base::File portfile(portpath, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!portfile.IsValid()) {
    VLOG(0) << "tor: failed to open control port";
    return false;
  }

  // Get the file's info, including modification time.
  base::File::Info info;
  if (!portfile.GetInfo(&info)) {
    VLOG(0) << "tor: failed to stat control port";
    return false;
  }

  // Read up to 27/28 octets, the maximum we will ever need.
  const size_t kBufSiz = strlen(kControlPortMaxTmpl);
  char buf[kBufSiz];
  int nread = portfile.ReadAtCurrentPos(buf, sizeof buf);
  if (nread < 0) {
    VLOG(0) << "tor: failed to read control port";
    return false;
  }
  if (static_cast<size_t>(nread) < strlen(kControlPortMinTmpl)) {
    VLOG(0) << "tor: control port truncated";
    return false;
  }
  DCHECK(static_cast<size_t>(nread) <= sizeof buf);

  std::string text(buf, 0, nread);

  // Sanity-check the content.
  if (!base::StartsWith(text, "PORT=", base::CompareCase::SENSITIVE) ||
      !base::EndsWith(text, kLineBreak, base::CompareCase::SENSITIVE)) {
    VLOG(0) << "tor: invalid control port: "
            << "`" << text << ";";  // XXX escape
    return false;
  }

  // Verify that it's localhost.
  const char expected[] = "PORT=127.0.0.1:";
  if (!base::StartsWith(text, expected, base::CompareCase::SENSITIVE)) {
    VLOG(0) << "tor: control port has non-local control address";
    return false;
  }

  // Parse it!
  std::string portstr(text, strlen(expected),
                      nread - strlen(kLineBreak) - strlen(expected));
  if (!base::StringToInt(portstr, &port)) {
    VLOG(0) << "tor: failed to parse control port: "
            << "`" << portstr << "'";  // XXX escape
    return false;
  }
  mtime = info.last_modified;
  VLOG(3) << "Control port " << port << ", mtime " << mtime;
  return true;
}

bool TorControl::EatOldPid(base::ProcessId* id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watch_sequence_checker_);
  DCHECK(id);

  // Open the tor pid file.
  base::FilePath pidpath = watch_dir_path_.AppendASCII(kTorPidName);
  base::File pidfile(pidpath, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!pidfile.IsValid()) {
    VLOG(0) << "tor: failed to open tor.pid";
    return false;
  }

  const size_t kBufSiz = pidfile.GetLength();
  char buf[kBufSiz];
  int nread = pidfile.ReadAtCurrentPos(buf, sizeof buf);
  if (nread < 0) {
    VLOG(0) << "tor: failed to read tor pid file";
    return false;
  }

  std::string pid(buf, 0, nread - strlen(kLineBreak));
#if defined(OS_WIN)
  *id = stoul(pid, nullptr);
#else
  if (!base::StringToInt(pid, id)) {
    VLOG(0) << "tor: failed to parse tor pid";
    return false;
  }
#endif
  return true;
}

// PollDone()
//
//      Just finished polling the watch directory and failed to
//      establish a connection.  Decide whether to go back to watching
//      and waiting or whether to poll again, if something else
//      happened on the file system while we were busy polling.
//
void TorControl::PollDone() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watch_sequence_checker_);
  DCHECK(polling_);

  if (repoll_) {
    VLOG(2) << "tor: retrying control connection";
    repoll_ = false;
    watch_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&TorControl::Poll, base::Unretained(this)));
  } else {
    VLOG(2) << "tor: control connection not yet ready";
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
void TorControl::OpenControl(int portno, std::vector<uint8_t> cookie) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  VLOG(3) << __func__ << " " << base::HexEncode(cookie.data(), cookie.size());

  net::AddressList addrlist = net::AddressList::CreateFromIPAddress(
      net::IPAddress::IPv4Localhost(), portno);
  socket_ = std::make_unique<net::TCPClientSocket>(
      addrlist, nullptr, nullptr, net::NetLog::Get(), net::NetLogSource());
  int rv = socket_->Connect(base::BindOnce(
      &TorControl::Connected, base::Unretained(this), std::move(cookie)));
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
void TorControl::Connected(std::vector<uint8_t> cookie, int rv) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);

  if (rv != net::OK) {
    // Connection failed but there may have been more watch directory
    // activity while we were waiting.  If so, try again; if not, go
    // back to watching and waiting.
    VLOG(1) << "tor: control connection failed: " << net::ErrorToString(rv);
    watch_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&TorControl::PollDone, base::Unretained(this)));
    return;
  }

  Cmd1("AUTHENTICATE " + base::HexEncode(cookie.data(), cookie.size()),
       base::BindOnce(&TorControl::Authenticated, base::Unretained(this)));
}

// Authenticated(error, status, reply)
//
//      Tor control AUTHENTICATE command callback.  If we failed, kill
//      the connection and start over.  If we succeeded, announce that
//      we're ready.
//
void TorControl::Authenticated(bool error,
                               const std::string& status,
                               const std::string& reply) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  if (!error) {
    if (status != "250" || reply != "OK")
      error = true;
  }
  if (error) {
    VLOG(0) << "tor: control authentication failed";
    return;
  }
  VLOG(2) << "tor: control connection ready";
  NotifyTorControlReady();
}

///////////////////////////////////////////////////////////////////////////////
// Event subscriptions

// Subscribe(event, callback)
//
//      Subscribe to event by sending SETEVENTS with it included
//      (along with all previously subscribed events).  If repeated,
//      just increment nesting depth without sending SETEVENTS.  Call
//      the callback once the subscription has been processed.
//      Subsequently, whenever the event happens, notify delegate the
//      OnTorEvent.
//
void TorControl::Subscribe(TorControlEvent event,
                           base::OnceCallback<void(bool error)> callback) {
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&TorControl::DoSubscribe, base::Unretained(this), event,
                     std::move(callback)));
}

void TorControl::DoSubscribe(TorControlEvent event,
                             base::OnceCallback<void(bool error)> callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  if (async_events_[event]++ != 0) {
    bool error = false;
    std::move(callback).Run(error);
    return;
  }

  async_events_[event] = 1;
  Cmd1(SetEventsCmd(),
       base::BindOnce(&TorControl::Subscribed, base::Unretained(this), event,
                      std::move(callback)));
}

void TorControl::Subscribed(TorControlEvent event,
                            base::OnceCallback<void(bool error)> callback,
                            bool error,
                            const std::string& status,
                            const std::string& reply) {
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
//      will not trigger notification of OnTorEvent.
//
void TorControl::Unsubscribe(TorControlEvent event,
                             base::OnceCallback<void(bool error)> callback) {
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&TorControl::DoUnsubscribe, base::Unretained(this), event,
                     std::move(callback)));
}

void TorControl::DoUnsubscribe(TorControlEvent event,
                               base::OnceCallback<void(bool error)> callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  // We had better already be subscribed.
  DCHECK_GE(async_events_[event], 1u);
  if (--async_events_[event] != 0) {
    bool error = false;
    std::move(callback).Run(error);
    return;
  }

  DCHECK_EQ(async_events_[event], 0u);
  async_events_.erase(event);
  Cmd1(SetEventsCmd(),
       base::BindOnce(&TorControl::Unsubscribed, base::Unretained(this), event,
                      std::move(callback)));
}

void TorControl::Unsubscribed(TorControlEvent event,
                              base::OnceCallback<void(bool error)> callback,
                              bool error,
                              const std::string& status,
                              const std::string& reply) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  DCHECK_EQ(async_events_.count(event), 0u);
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
std::string TorControl::SetEventsCmd() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  std::ostringstream cmds;
  cmds << "SETEVENTS";
  for (const auto& entry : async_events_) {
    const auto& found = kTorControlEventByEnum.find(entry.first);
    DCHECK(found != kTorControlEventByEnum.end());
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
void TorControl::Cmd1(const std::string& cmd, CmdCallback callback) {
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
void TorControl::Cmd(const std::string& cmd,
                     PerLineCallback perline,
                     CmdCallback callback) {
  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&TorControl::DoCmd, base::Unretained(this), cmd,
                                std::move(perline), std::move(callback)));
}

void TorControl::DoCmd(std::string cmd,
                       PerLineCallback perline,
                       CmdCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  NotifyTorRawCmd(cmd);
  if (!socket_ || writeq_.size() > 100 || cmdq_.size() > 100) {
    // Socket is closed, or over 100 commands pending or synchronous
    // callbacks queued -- something is probably wrong.
    bool error = true;
    std::move(callback).Run(error, "", "");
    return;
  }
  writeq_.push(cmd + "\r\n");
  cmdq_.push(std::make_pair(std::move(perline), std::move(callback)));
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
void TorControl::GetVersion(
    base::OnceCallback<void(bool error, const std::string& version)> callback) {
  std::unique_ptr<std::string> version = std::make_unique<std::string>();
  std::string* versionp = version.get();
  Cmd(kGetVersionCmd,
      base::BindRepeating(&TorControl::GetVersionLine, base::Unretained(this),
                          versionp),
      base::BindOnce(&TorControl::GetVersionDone, base::Unretained(this),
                     std::move(version), std::move(callback)));
}

void TorControl::GetVersionLine(std::string* version,
                                const std::string& status,
                                const std::string& reply) {
  if (status != "250" ||
      !base::StartsWith(reply, kGetVersionReply,
                        base::CompareCase::SENSITIVE) ||
      !version->empty()) {
    VLOG(0) << "tor: unexpected " << kGetVersionCmd << " reply";
    return;
  }
  *version = reply.substr(strlen(kGetVersionReply));
}

void TorControl::GetVersionDone(
    std::unique_ptr<std::string> version,
    base::OnceCallback<void(bool error, const std::string& version)> callback,
    bool error,
    const std::string& status,
    const std::string& reply) {
  if (error || status != "250" || reply != "OK" || version->empty()) {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), true, ""));
    return;
  }
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), false, *version));
}

void TorControl::GetSOCKSListeners(
    base::OnceCallback<
        void(bool error, const std::vector<std::string>& listeners)> callback) {
  std::unique_ptr<std::vector<std::string>> listeners =
      std::make_unique<std::vector<std::string>>();
  std::vector<std::string>* listeners_p = listeners.get();
  Cmd(kGetSOCKSListenersCmd,
      base::BindRepeating(&TorControl::GetSOCKSListenersLine,
                          base::Unretained(this), listeners_p),
      base::BindOnce(&TorControl::GetSOCKSListenersDone, base::Unretained(this),
                     std::move(listeners), std::move(callback)));
}

void TorControl::GetSOCKSListenersLine(std::vector<std::string>* listeners,
                                       const std::string& status,
                                       const std::string& reply) {
  if (status != "250" || !base::StartsWith(reply, kGetSOCKSListenersReply,
                                           base::CompareCase::SENSITIVE)) {
    VLOG(0) << "tor: unexpected " << kGetSOCKSListenersCmd << " reply";
    return;
  }
  listeners->push_back(reply.substr(strlen(kGetSOCKSListenersReply)));
}

void TorControl::GetSOCKSListenersDone(
    std::unique_ptr<std::vector<std::string>> listeners,
    base::OnceCallback<
        void(bool error, const std::vector<std::string>& listeners)> callback,
    bool error,
    const std::string& status,
    const std::string& reply) {
  if (error || status != "250" || reply != "OK" || listeners->empty()) {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), true, std::vector<std::string>()));
    return;
  }
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), false, *listeners));
}

///////////////////////////////////////////////////////////////////////////////
// Writing state machine

// StartWrite()
//
//      Pick a write off the queue and start an I/O buffer for it.
//
//      Caller must ensure writing_ is true.
//
void TorControl::StartWrite() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  DCHECK(writing_);
  DCHECK(!writeq_.empty());
  DCHECK(!cmdq_.empty());
  auto buf = base::MakeRefCounted<net::StringIOBuffer>(writeq_.front());
  writeiobuf_ = base::MakeRefCounted<net::DrainableIOBuffer>(buf, buf->size());
  writeq_.pop();
}

// DoWrites()
//
//      Issue writes from writeiobuf_, and arrange to issue the rest
//      of the writes in the queue when done.
//
//      Caller must ensure writing_ is true and writeiobuf_ is
//      initialized.
//
void TorControl::DoWrites() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  DCHECK(writing_);
  DCHECK(writeiobuf_);
  int rv;
  while (
      (rv = socket_->Write(
           writeiobuf_.get(), writeiobuf_->size(),
           base::BindOnce(&TorControl::WriteDoneAsync, base::Unretained(this)),
           tor_control_traffic_annotation)) != net::ERR_IO_PENDING) {
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
void TorControl::WriteDoneAsync(int rv) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  DCHECK(writing_);
  DCHECK(writeiobuf_);
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
void TorControl::WriteDone(int rv) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  DCHECK(writing_);
  DCHECK(writeiobuf_);
  // Bail if error.
  if (rv < 0) {
    VLOG(1) << "tor: control write error: " << net::ErrorToString(rv);
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
void TorControl::StartRead() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  DCHECK(reading_);
  DCHECK(!cmdq_.empty() || !async_events_.empty());
  readiobuf_ = base::MakeRefCounted<net::GrowableIOBuffer>();
  readiobuf_->SetCapacity(kTorBufferSize);
  read_start_ = 0;
  DCHECK(readiobuf_->RemainingCapacity());
}

// DoReads()
//
//      Issue reads into readiobuf_ and process them.
//
//      Caller must ensure reading_ is true and readiobuf_ is
//      initialized.
//
void TorControl::DoReads() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  DCHECK(reading_);
  DCHECK(readiobuf_);
  int rv;
  DCHECK(readiobuf_->RemainingCapacity());
  while ((rv = socket_->Read(readiobuf_.get(), readiobuf_->RemainingCapacity(),
                             base::BindOnce(&TorControl::ReadDoneAsync,
                                            base::Unretained(this)))) !=
         net::ERR_IO_PENDING) {
    ReadDone(rv);
    if (!reading_)
      break;
    DCHECK(readiobuf_->RemainingCapacity());
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
void TorControl::ReadDoneAsync(int rv) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  DCHECK(reading_);
  DCHECK(readiobuf_);
  ReadDone(rv);
  if (reading_) {
    DCHECK(readiobuf_->RemainingCapacity());
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
void TorControl::ReadDone(int rv) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  DCHECK(reading_);
  DCHECK(readiobuf_);
  if (rv < 0) {
    VLOG(1) << "tor: control read error: " << net::ErrorToString(rv);
    Error();
    return;
  }
  if (rv == 0) {
    VLOG(1) << "tor: control closed prematurely";
    Error();
    return;
  }
  const char* data = readiobuf_->data();
  for (int i = 0; i < rv; i++) {
    if (!read_cr_) {
      // No CR yet.  Accept CR or non-LF; reject LF.
      if (data[i] == 0x0d) {  // CR
        read_cr_ = true;
      } else if (data[i] == 0x0a) {  // LF
        VLOG(1) << "tor: stray line feed";
        Error();
        return;
      } else {
        // Anything else: Just accept it and move on.
      }
    } else {
      // CR seen.  Accept LF; reject all else.
      if (data[i] == 0x0a) {  // LF
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
        VLOG(1) << "tor: stray carriage return";
        Error();
        return;
      }
    }
  }

  // If we've walked up to the end of the buffer, try shifting it to
  // the beginning to make room; if there's no more room, fail --
  // lines shouldn't be this long.
  DCHECK(rv <= readiobuf_->RemainingCapacity());
  if (readiobuf_->RemainingCapacity() == rv) {
    if (read_start_ == 0) {
      // Line is too long.
      VLOG(1) << "tor: control line too long";
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
  DCHECK(readiobuf_->RemainingCapacity());

  // If we've processed every byte in the input so far, and there's no
  // more command callbacks queued or asynchronous events registered,
  // stop.
  if (read_start_ == readiobuf_->offset() && cmdq_.empty() &&
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
bool TorControl::ReadLine(const std::string& line) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);

  if (line.size() < 4) {
    // Line is too short.
    VLOG(1) << "tor: control line too short";
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
    // Notify delegate of the raw reply.
    NotifyTorRawAsync(status, reply);

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
            VLOG(1) << "tor: unknown event: " << event_name;  // XXX escape
            return false;
          }
          const TorControlEvent event = (*found).second;

          // Ignore if we don't think we're subscribed to this.
          if (!async_events_.count(event)) {
            VLOG(1) << "tor: spurious event: " << event_name;
            return true;
          }

          // Notify the delegate of the parsed reply.  No extra
          // because there were no intermediate reply lines.
          NotifyTorEvent(event, initial, {});

          return true;
        }
        case '-': {
          // Start of a multi-line async reply.

          // Start a fresh async reply state.  Parse the rest, but
          // skip it, if we don't recognize the event.
          const auto& found = kTorControlEventByName.find(event_name);
          const TorControlEvent event =
              (found == kTorControlEventByName.end() ? TorControlEvent::INVALID
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
          if (!ParseKV(reply, &key, &value)) {
            VLOG(1) << "tor: invalid async continuation line";
            Error();
            return false;
          }
          if (async_->extra.count(key)) {
            VLOG(1) << "tor: duplicate key in async continuation line";
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
            if (!ParseKV(reply, &key, &value)) {
              VLOG(1) << "tor: invalid async event";
              Error();
              return false;
            }
            if (async_->extra.count(key)) {
              VLOG(1) << "tor: duplicate key in async event";
              Error();
              return false;
            }
            async_->extra[key] = value;

            // If we're still subscribed, notify the delegate of the
            // parsed reply.
            if (async_events_.count(async_->event)) {
              NotifyTorEvent(async_->event, async_->initial, async_->extra);
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
        NotifyTorRawMid(status, reply);
        if (!cmdq_.empty()) {
          PerLineCallback& perline = cmdq_.front().first;
          perline.Run(status, reply);
        }
        return true;
      case '+':
        VLOG(2) << "tor: NYI: control data reply";
        // XXX Just ignore it for now.
        return true;
      case ' ':
        NotifyTorRawEnd(status, reply);
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
  VLOG(1) << "tor: malformed control line: "
          << escapify(line.data(), line.size());
  Error();
  return false;
}

TorControl::Async::Async() = default;
TorControl::Async::~Async() = default;

// Error()
//
//      Clear read and write state and disconnect.
//
void TorControl::Error() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);

  VLOG(1) << "tor: closing control on " << (running_ ? "request" : "error");

  NotifyTorClosed();

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
        FROM_HERE, base::BindOnce(&TorControl::Poll, base::Unretained(this)));
  }
}

void TorControl::NotifyTorControlReady() {
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<TorControl::Delegate> delegate) {
                       if (delegate)
                         delegate->OnTorControlReady();
                     },
                     delegate_->AsWeakPtr()));
}

void TorControl::NotifyTorClosed() {
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<TorControl::Delegate> delegate) {
                       if (delegate)
                         delegate->OnTorClosed();
                     },
                     delegate_->AsWeakPtr()));
}

void TorControl::NotifyTorCleanupNeeded(base::ProcessId id) {
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](base::WeakPtr<TorControl::Delegate> delegate, base::ProcessId id) {
            if (delegate)
              delegate->OnTorCleanupNeeded(std::move(id));
          },
          delegate_->AsWeakPtr(), std::move(id)));
}

void TorControl::NotifyTorEvent(
    TorControlEvent event,
    const std::string& initial,
    const std::map<std::string, std::string>& extra) {
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<TorControl::Delegate> delegate,
                        TorControlEvent event, const std::string& initial,
                        const std::map<std::string, std::string>& extra) {
                       if (delegate)
                         delegate->OnTorEvent(event, initial, extra);
                     },
                     delegate_->AsWeakPtr(), event, initial, extra));
}

void TorControl::NotifyTorRawCmd(const std::string& cmd) {
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<TorControl::Delegate> delegate,
                        const std::string& cmd) {
                       if (delegate)
                         delegate->OnTorRawCmd(cmd);
                     },
                     delegate_->AsWeakPtr(), cmd));
}

void TorControl::NotifyTorRawAsync(const std::string& status,
                                   const std::string& line) {
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<TorControl::Delegate> delegate,
                        const std::string& status, const std::string& line) {
                       if (delegate)
                         delegate->OnTorRawAsync(status, line);
                     },
                     delegate_->AsWeakPtr(), status, line));
}

void TorControl::NotifyTorRawMid(const std::string& status,
                                 const std::string& line) {
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<TorControl::Delegate> delegate,
                        const std::string& status, const std::string& line) {
                       if (delegate)
                         delegate->OnTorRawMid(status, line);
                     },
                     delegate_->AsWeakPtr(), status, line));
}

void TorControl::NotifyTorRawEnd(const std::string& status,
                                 const std::string& line) {
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<TorControl::Delegate> delegate,
                        const std::string& status, const std::string& line) {
                       if (delegate)
                         delegate->OnTorRawEnd(status, line);
                     },
                     delegate_->AsWeakPtr(), status, line));
}

// ParseKV(string, key, value)
//
//      Parse KEY=VALUE notation from string into key and value,
//      following the Tor control spec notation.  Return true on
//      success, false on failure.
//
// static
bool TorControl::ParseKV(const std::string& string,
                         std::string* key,
                         std::string* value) {
  size_t end;
  return ParseKV(string, key, value, &end) && end == string.size();
}

// ParseKV(string, key, value, end)
//
//      Parse KEY=VALUE notation from string into key and value,
//      following the Tor control spec notation, and set end to the
//      number of octets consumed.  Return true on success, false on
//      failure.
//
// static
bool TorControl::ParseKV(const std::string& string,
                         std::string* key,
                         std::string* value,
                         size_t* end) {
  DCHECK(key && value && end);
  // Search for `=' -- it had better be there.
  size_t eq = string.find("=");
  if (eq == std::string::npos)
    return false;
  size_t vstart = eq + 1;

  // If we're at the end of the string, value is empt.
  if (vstart == string.size()) {
    *key = string.substr(0, eq);
    *value = "";
    *end = string.size();
    return true;
  }

  // Check whether it's quoted.
  if (string[vstart] != '"') {
    // Not quoted.  Check for a delimiter.
    size_t i, vend = string.size();
    if ((i = string.find(" ", vstart)) != std::string::npos) {
      // Delimited.  Stop at the delimiter, and consume it.
      vend = i;
      *end = vend + 1;
    } else {
      // Not delimited.  Stop at the end of string.
      *end = vend;
    }

    // Check for internal quotes; they are forbidden.
    if ((i = string.find("\"", vstart)) != std::string::npos)
      return false;

    // Extract the key and value and we're done.
    *key = string.substr(0, eq);
    *value = string.substr(vstart, vend - vstart);
    return true;
  }

  // Quoted string.  Parse it, and consume trailing spaces.
  if (!ParseQuoted(string.substr(eq + 1), value, end))
    return false;
  *key = string.substr(0, eq);
  *end += eq + 1;
  while (*end < string.size() && string[*end] == ' ')
    (*end)++;
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
bool TorControl::ParseQuoted(const std::string& string,
                             std::string* value,
                             size_t* end) {
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
        DCHECK(0);
        S = REJECT;
        break;
      case START:
        S = (ch == '"' ? BODY : REJECT);
        break;
      case BODY:
        switch (ch) {
          case '\\':
            S = BACKSLASH;
            break;
          case '"':
            S = ACCEPT;
            break;
          default:
            buf[pos++] = ch;
            S = BODY;
            break;
        }
        break;
      case BACKSLASH:
        switch (ch) {
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
            octal = (ch - '0') << 6;
            S = OCTAL1;
            break;
          case 'n':
            buf[pos++] = '\n';
            S = BODY;
            break;
          case 'r':
            buf[pos++] = '\r';
            S = BODY;
            break;
          case 't':
            buf[pos++] = '\t';
            S = BODY;
            break;
          case '\\':
          case '"':
          case '\'':
            buf[pos++] = ch;
            S = BODY;
            break;
          default:
            S = REJECT;
            break;
        }
        break;
      case OCTAL1:
        switch (ch) {
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
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
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
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
        *value = buf.substr(0, pos);
        *end = i + 1;
        return true;
      default:
        break;
    }
  }

  // Consumed the whole string without accepting it.  Reject!
  return false;
}

}  // namespace tor
