/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "brave/components/tor/tor_control.h"

#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/sequence_checker.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/task/sequenced_task_runner.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/socket/tcp_client_socket.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

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

constexpr char kGetVersionCmd[] = "GETINFO version";
constexpr char kGetVersionReply[] = "version=";
constexpr char kGetSOCKSListenersCmd[] = "GETINFO net/listeners/socks";
constexpr char kGetSOCKSListenersReply[] = "net/listeners/socks=";
constexpr char kGetCircuitEstablishedCmd[] =
    "GETINFO status/circuit-established";
constexpr char kGetCircuitEstablishedReply[] = "status/circuit-established=";

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

TorControl::TorControl(base::WeakPtr<TorControl::Delegate> delegate,
                       scoped_refptr<base::SequencedTaskRunner> task_runner)
    : running_(false),
      owner_task_runner_(base::SequencedTaskRunner::GetCurrentDefault()),
      io_task_runner_(task_runner),
      writing_(false),
      reading_(false),
      read_start_(0u),
      read_cr_(false),
      delegate_(delegate) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owner_sequence_checker_);
  DETACH_FROM_SEQUENCE(io_sequence_checker_);
}

TorControl::~TorControl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
}

// Start()
//
//      Start watching for the Tor control channel.  If we are able to
//      connect, issue OnTorControlReady to delegate.
//
void TorControl::Start(std::vector<uint8_t> cookie, int port) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owner_sequence_checker_);
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&TorControl::OpenControl, weak_ptr_factory_.GetWeakPtr(),
                     port, std::move(cookie)));
}

// Stop()
//
//      Stop watching for the Tor control channel, and disconnect if
//      we have already connected.
//
void TorControl::Stop() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owner_sequence_checker_);
  io_task_runner_->PostTask(FROM_HERE,
                            base::BindOnce(&TorControl::StopOnTaskRunner,
                                           weak_ptr_factory_.GetWeakPtr()));
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
  DCHECK(!running_);

  running_ = true;
  VLOG(3) << __func__ << " " << base::HexEncode(cookie.data(), cookie.size());

  net::AddressList addrlist = net::AddressList::CreateFromIPAddress(
      net::IPAddress::IPv4Localhost(), portno);
  socket_ = std::make_unique<net::TCPClientSocket>(
      addrlist, nullptr, nullptr, net::NetLog::Get(), net::NetLogSource());
  int rv = socket_->Connect(base::BindOnce(
      &TorControl::Connected, weak_ptr_factory_.GetWeakPtr(), cookie));
  if (rv == net::ERR_IO_PENDING)
    return;
  Connected(std::move(cookie), rv);
}

void TorControl::StopOnTaskRunner() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  if (!running_)
    return;

  running_ = false;
  async_events_.clear();
  Error();
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
    NotifyTorControlClosed();
    return;
  }

  DoCmd("AUTHENTICATE " + base::HexEncode(cookie.data(), cookie.size()),
        base::DoNothing(),
        base::BindOnce(&TorControl::Authenticated,
                       weak_ptr_factory_.GetWeakPtr()));
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

  DoCmd("TAKEOWNERSHIP", base::DoNothing(), base::DoNothing());
  DoCmd("RESETCONF __OwningControllerProcess", base::DoNothing(),
        base::DoNothing());
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
  DCHECK_CALLED_ON_VALID_SEQUENCE(owner_sequence_checker_);
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&TorControl::DoSubscribe, weak_ptr_factory_.GetWeakPtr(),
                     event, std::move(callback)));
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
  DoCmd(SetEventsCmd(), base::DoNothing(),
        base::BindOnce(&TorControl::Subscribed, weak_ptr_factory_.GetWeakPtr(),
                       event, std::move(callback)));
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
  DCHECK_CALLED_ON_VALID_SEQUENCE(owner_sequence_checker_);
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&TorControl::DoUnsubscribe, weak_ptr_factory_.GetWeakPtr(),
                     event, std::move(callback)));
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
  DoCmd(
      SetEventsCmd(), base::DoNothing(),
      base::BindOnce(&TorControl::Unsubscribed, weak_ptr_factory_.GetWeakPtr(),
                     event, std::move(callback)));
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

// DoCmd(cmd, perline, callback)
//
//      Issue a Tor control command.  Call perline for each
//      intermediate line; then call callback for the last line or on
//      error.
//
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
  DCHECK_CALLED_ON_VALID_SEQUENCE(owner_sequence_checker_);
  std::unique_ptr<std::string> version = std::make_unique<std::string>();
  std::string* version_p = version.get();
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &TorControl::DoCmd, weak_ptr_factory_.GetWeakPtr(), kGetVersionCmd,
          base::BindRepeating(&TorControl::GetVersionLine,
                              weak_ptr_factory_.GetWeakPtr(), version_p),
          base::BindOnce(&TorControl::GetVersionDone,
                         weak_ptr_factory_.GetWeakPtr(), std::move(version),
                         std::move(callback))));
}

void TorControl::GetVersionLine(std::string* version,
                                const std::string& status,
                                const std::string& reply) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  if (status != "250" || !reply.starts_with(kGetVersionReply) ||
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
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  if (error || status != "250" || reply != "OK" || version->empty()) {
    std::move(callback).Run(true, "");
    return;
  }
  std::move(callback).Run(false, *version);
}

void TorControl::GetSOCKSListeners(
    base::OnceCallback<
        void(bool error, const std::vector<std::string>& listeners)> callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owner_sequence_checker_);
  std::unique_ptr<std::vector<std::string>> listeners =
      std::make_unique<std::vector<std::string>>();
  std::vector<std::string>* listeners_p = listeners.get();
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &TorControl::DoCmd, weak_ptr_factory_.GetWeakPtr(),
          kGetSOCKSListenersCmd,
          base::BindRepeating(&TorControl::GetSOCKSListenersLine,
                              weak_ptr_factory_.GetWeakPtr(), listeners_p),
          base::BindOnce(&TorControl::GetSOCKSListenersDone,
                         weak_ptr_factory_.GetWeakPtr(), std::move(listeners),
                         std::move(callback))));
}

void TorControl::GetSOCKSListenersLine(std::vector<std::string>* listeners,
                                       const std::string& status,
                                       const std::string& reply) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  if (status != "250" || !reply.starts_with(kGetSOCKSListenersReply)) {
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
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  if (error || status != "250" || reply != "OK" || listeners->empty()) {
    std::move(callback).Run(true, std::vector<std::string>());
    return;
  }
  std::move(callback).Run(false, *listeners);
}

void TorControl::GetCircuitEstablished(
    base::OnceCallback<void(bool error, bool established)> callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owner_sequence_checker_);
  std::unique_ptr<std::string> established = std::make_unique<std::string>();
  std::string* established_p = established.get();
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &TorControl::DoCmd, weak_ptr_factory_.GetWeakPtr(),
          kGetCircuitEstablishedCmd,
          base::BindRepeating(&TorControl::GetCircuitEstablishedLine,
                              weak_ptr_factory_.GetWeakPtr(), established_p),
          base::BindOnce(&TorControl::GetCircuitEstablishedDone,
                         weak_ptr_factory_.GetWeakPtr(), std::move(established),
                         std::move(callback))));
}

void TorControl::GetCircuitEstablishedLine(std::string* established,
                                           const std::string& status,
                                           const std::string& reply) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  if (status != "250" || !reply.starts_with(kGetCircuitEstablishedReply) ||
      !established->empty()) {
    VLOG(0) << "tor: unexpected " << kGetCircuitEstablishedCmd << " reply";
    return;
  }
  *established = reply.substr(strlen(kGetCircuitEstablishedReply));
}

void TorControl::GetCircuitEstablishedDone(
    std::unique_ptr<std::string> established,
    base::OnceCallback<void(bool error, bool established)> callback,
    bool error,
    const std::string& status,
    const std::string& reply) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  bool result;
  if (*established == "1")
    result = true;
  else if (*established == "0")
    result = false;
  else
    error = true;
  if (error || status != "250" || reply != "OK" || established->empty()) {
    std::move(callback).Run(true, false);
    return;
  }
  std::move(callback).Run(false, result);
}

void TorControl::SetupPluggableTransport(
    const base::FilePath& snowflake,
    const base::FilePath& obfs4,
    base::OnceCallback<void(bool error)> callback) {
  if (snowflake.empty() || obfs4.empty())
    return;

  if (owner_task_runner_->RunsTasksInCurrentSequence()) {
    io_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&TorControl::SetupPluggableTransport,
                                  weak_ptr_factory_.GetWeakPtr(), snowflake,
                                  obfs4, std::move(callback)));
    return;
  }
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);

  const auto snowflake_path =
      base::FilePath::FromASCII("../../").Append(snowflake);
  const auto obfs4_path = base::FilePath::FromASCII("../../").Append(obfs4);

  constexpr const char kObfs4ConfigCmd[] =
      "ClientTransportPlugin=\"meek_lite,obfs2,obfs3,obfs4,scramblesuit "
      "exec %s\"";
  constexpr const char kSnowflakeConfigCmd[] =
      "ClientTransportPlugin=\"snowflake exec %s -url "
      "https://snowflake-broker.torproject.net.global.prod.fastly.net/ "
      "-front cdn.sstatic.net "
      "-ice "
      "stun:stun.l.google.com:19302,stun:stun.voip.blackberry.com:3478,stun:"
      "stun.altar.com.pl:3478,stun:stun.antisip.com:3478,stun:stun.bluesip.net:"
      "3478,stun:stun.dus.net:3478,stun:stun.epygi.com:3478,stun:stun.sonetel."
      "com:3478,stun:stun.sonetel.net:3478,stun:stun.stunprotocol.org:3478,"
      "stun:stun.uls.co.za:3478,stun:stun.voipgate.com:3478,stun:stun.voys.nl:"
      "3478\"";

  const std::string snowflake_setup = base::StringPrintf(
      kSnowflakeConfigCmd,
      snowflake_path.NormalizePathSeparatorsTo(FILE_PATH_LITERAL('/'))
          .AsUTF8Unsafe()
          .c_str());
  const std::string obfs4_setup = base::StringPrintf(
      kObfs4ConfigCmd,
      obfs4_path.NormalizePathSeparatorsTo(FILE_PATH_LITERAL('/'))
          .AsUTF8Unsafe()
          .c_str());

  const std::string configure_pluggable_transport =
      base::StrCat({"SETCONF ", snowflake_setup, " ", obfs4_setup});

  DoCmd(configure_pluggable_transport, base::DoNothing(),
        base::BindOnce(&TorControl::OnPluggableTransportsConfigured,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void TorControl::SetupBridges(const std::vector<std::string>& bridges,
                              base::OnceCallback<void(bool error)> callback) {
  if (owner_task_runner_->RunsTasksInCurrentSequence()) {
    io_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&TorControl::SetupBridges,
                                  weak_ptr_factory_.GetWeakPtr(), bridges,
                                  std::move(callback)));
    return;
  }
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  if (bridges.empty()) {
    DoCmd("RESETCONF UseBridges Bridge ClientTransportPlugin",
          base::DoNothing(),
          base::BindOnce(&TorControl::OnBrigdesConfigured,
                         weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  } else {
    std::string command = "SETCONF ";
    for (const auto& bridge : bridges) {
      base::StrAppend(&command, {"Bridge=\"", bridge, "\""});
    }
    base::StrAppend(&command, {"UseBridges=1"});
    DoCmd(std::move(command), base::DoNothing(),
          base::BindOnce(&TorControl::OnBrigdesConfigured,
                         weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }
}

void TorControl::OnPluggableTransportsConfigured(
    base::OnceCallback<void(bool error)> callback,
    bool error,
    const std::string& status,
    const std::string& reply) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  VLOG(1) << __func__ << " " << reply;
  std::move(callback).Run(error || status != "250" || reply != "OK");
}

void TorControl::OnBrigdesConfigured(
    base::OnceCallback<void(bool error)> callback,
    bool error,
    const std::string& status,
    const std::string& reply) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  VLOG(1) << __func__ << " " << reply;
  std::move(callback).Run(error || status != "250" || reply != "OK");
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
  while ((rv = socket_->Write(writeiobuf_.get(), writeiobuf_->size(),
                              base::BindOnce(&TorControl::WriteDoneAsync,
                                             weak_ptr_factory_.GetWeakPtr()),
                              tor_control_traffic_annotation)) !=
         net::ERR_IO_PENDING) {
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
  read_start_ = 0u;
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
                                            weak_ptr_factory_.GetWeakPtr()))) !=
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
        std::string_view line =
            base::as_string_view(readiobuf_->everything().subspan(
                read_start_, readiobuf_->offset() + i - 1 - read_start_));
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
    readiobuf_->everything().copy_prefix_from(readiobuf_->everything().subspan(
        read_start_, readiobuf_->offset() + rv - read_start_));
    readiobuf_->set_offset(readiobuf_->offset() + rv - read_start_);
    read_start_ = 0;
  } else {
    // Otherwise, just advance the offset by the size of this input.
    readiobuf_->set_offset(readiobuf_->offset() + rv);
  }
  DCHECK(readiobuf_->RemainingCapacity());

  // If we've processed every byte in the input so far, and there's no
  // more command callbacks queued or asynchronous events registered,
  // stop.
  if (read_start_ == base::checked_cast<size_t>(readiobuf_->offset()) &&
      cmdq_.empty() && async_events_.empty()) {
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
bool TorControl::ReadLine(std::string_view line) {
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
  std::string status(line.substr(0, 3));
  char pos = line[3];
  std::string reply(line.substr(4));

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

  NotifyTorControlClosed();

  // Invoke all callbacks with errors and clear read state.
  while (!cmdq_.empty()) {
    CmdCallback& callback = cmdq_.front().second;
    bool error = true;
    std::move(callback).Run(error, "", "");
    cmdq_.pop();
  }
  reading_ = false;
  readiobuf_.reset();
  read_start_ = 0u;
  read_cr_ = false;

  // Clear write state.
  writeq_ = {};
  writing_ = false;
  writeiobuf_.reset();

  // Clear the socket.
  socket_.reset();
}

void TorControl::NotifyTorControlReady() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  owner_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&Delegate::OnTorControlReady, delegate_));
}

void TorControl::NotifyTorControlClosed() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  owner_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&Delegate::OnTorControlClosed, delegate_, running_));
}

void TorControl::NotifyTorEvent(
    TorControlEvent event,
    const std::string& initial,
    const std::map<std::string, std::string>& extra) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  owner_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&Delegate::OnTorEvent, delegate_, event, initial, extra));
}

void TorControl::NotifyTorRawCmd(const std::string& cmd) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  owner_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&Delegate::OnTorRawCmd, delegate_, cmd));
}

void TorControl::NotifyTorRawAsync(const std::string& status,
                                   const std::string& line) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  owner_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&Delegate::OnTorRawAsync, delegate_, status, line));
}

void TorControl::NotifyTorRawMid(const std::string& status,
                                 const std::string& line) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  owner_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&Delegate::OnTorRawMid, delegate_, status, line));
}

void TorControl::NotifyTorRawEnd(const std::string& status,
                                 const std::string& line) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_sequence_checker_);
  owner_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&Delegate::OnTorRawEnd, delegate_, status, line));
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
    if (string.find("\"", vstart) != std::string::npos)
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
