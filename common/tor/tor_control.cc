/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/tor/tor_control.h"

#include "brave/common/tor/tor_control_observer.h"

#include "base/files/file.h"
#include "base/files/file_path_watcher.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "net/base/io_buffer.h"
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

}

TorControl::TorControl(scoped_refptr<base::SequencedTaskRunner> task_runner)
  : task_runner_(std::move(task_runner)),
    state_(STOPPED),
    writing_(false),
    reading_(false),
    read_start_(-1) {
}

TorControl::~TorControl() = default;

void TorControl::AddObserver(TorControlObserver* observer) {
  observers_.AddObserver(observer);
}

void TorControl::RemoveObserver(TorControlObserver* observer) {
  observers_.RemoveObserver(observer);
}

// Start()
//
//      Start watching for the Tor control channel.  If we are able to
//      connect, eventually issue TorControlObserver::OnTorReady to
//      all observers.
//
void TorControl::Start(const base::FilePath& watchDirPath) {
  CHECK(state_ == STOPPED);
  CHECK(watch_dir_path_.empty());
  CHECK(!watcher_);
  watcher_ = std::make_unique<base::FilePathWatcher>();
  watch_dir_path_ = watchDirPath;
  if (!watcher_->Watch(watch_dir_path_,
                       /*recursive*/ false,
                       base::BindRepeating(&TorControl::WatchDirChanged,
                                           base::Unretained(this)))) {
    LOG(ERROR) << "Failed to watch tor directory\n";
    watcher_.reset();
    return;
  }
  state_ = CONNECTING;
  PostPoll();
}

// Stop()
//
//      Stop watching for the Tor control channel, and disconnect if
//      we have already connected.
//
void TorControl::Stop() {
  state_ = STOPPED;
  watcher_.reset();
  watch_dir_path_.clear();
  Error();
  CHECK(state_ == STOPPED);
}

// WatchDirChanged(path, error)
//
//      Something happened in the watch directory at path.  Figure out
//      what happened and act on it.
//
void TorControl::WatchDirChanged(const base::FilePath& path, bool error) {
  // What state are we in?
  switch (state_) {
    case STOPPED:
      // Stopped.  The callback may have been queued up to run when
      // someone hit Stop().
      break;
    case WATCHING:
      // Watching and waiting.  Try connecting.
      state_ = CONNECTING;
      Poll();
      break;
    case CONNECTING:
      // We are connecting to the control socket.  Make sure to poll
      // again once we're done if anything went wrong connecting -- it
      // may be ready now.
      state_ = CONNECTING_REPOLL;
      break;
    case CONNECTING_REPOLL:
      // Already arranging to poll again, nothing more to do.
      break;
    case CONNECTED:
      // We are already connected, so there's no need to do anything
      // at this point.
      break;
  }
}

// PostPoll()
//
//      Post a task to call Poll().
//
void TorControl::PostPoll() {
  task_runner_->PostTask(FROM_HERE,
                         base::BindOnce(&TorControl::Poll,
                                        base::Unretained(this)));
}

// Poll()
//
//      Something happened in the watch directory.  See whether we
//      have a control cookie and control port to connect to, and if
//      so, start connecting.
//
void TorControl::Poll() {
  std::string cookie;
  base::Time cookie_mtime;
  int port;
  base::Time port_mtime;

  // We had better either be connecting or stopped.  If we were
  // stopped, stop here.
  CHECK(state_ == CONNECTING || state_ == STOPPED);
  if (state_ == STOPPED)
    return;

  do {
    if (!EatControlCookie(cookie, cookie_mtime))
      break;
    if (!EatControlPort(port, port_mtime))
      break;

    // Tor writes the control port first, then the auth cookie.
    // If the auth cookie is _older_ than the control port, then
    // it's definitely stale.  If they are the _same age_, then
    // probably the control port is older but the file system
    // resolution is just not enough to distinguish them.
    if (cookie_mtime < port_mtime) {
      LOG(ERROR) << "tor: tossing stale cookie\n";
      break;
    }

    OpenControl(port, cookie);
    return;
  } while (0);

  // We polled and failed.  If we need to retry polling, do so.
  Polled();
}

// EatControlCookie(cookie, mtime)
//
//      Try to read the control auth cookie.  Return true and set
//      cookie and mtime if successful; return false on failure.
//
bool TorControl::EatControlCookie(std::string& cookie, base::Time& mtime) {
  CHECK(state_ == CONNECTING || state_ == STOPPED);
  if (state_ == STOPPED)
    return false;
  // Open the control auth cookie file.
  base::FilePath cookiepath = watch_dir_path_.Append("control_auth_cookie");
  base::File cookiefile(cookiepath,
                        base::File::FLAG_OPEN|base::File::FLAG_READ);
  if (!cookiefile.IsValid())
    return false;

  // Get the file's info, including modification time.
  base::File::Info info;
  if (!cookiefile.GetInfo(&info))
    return false;

  // Read up to 33 octets.  We should need no more than 32, so 33 will
  // indicate the file is abnormally large.
  constexpr size_t bufsiz = 33;
  char buf[bufsiz];
  int nread = cookiefile.ReadAtCurrentPos(buf, bufsiz);
  if (nread < 0) {
    LOG(ERROR) << "Failed to read Tor control auth cookie\n";
    return false;
  }
  if (nread > 32) {
    LOG(ERROR) << "Tor control auth cookie too large\n";
    return false;
  }

  // Success!
  cookie.assign(buf, 0, nread);
  mtime = info.last_accessed;
  return true;
}

// EatControlPort(port, mtime)
//
//      Try to read the control port number.  Return true and set
//      port and mtime if successful; return false on failure.
//
bool TorControl::EatControlPort(int& port, base::Time& mtime) {
  CHECK(state_ == CONNECTING || state_ == STOPPED);
  if (state_ == STOPPED)
    return false;
  // Open the control port file.
  base::FilePath portpath = watch_dir_path_.Append("controlport");
  base::File portfile(portpath, base::File::FLAG_OPEN|base::File::FLAG_READ);
  if (!portfile.IsValid())
    return false;

  // Get the file's info, including modification time.
  base::File::Info info;
  if (!portfile.GetInfo(&info))
    return false;

  // Read up to 27 octets, the maximum we will ever need.
  const char mintmpl[] = "PORT=1.1.1.1:1\n";
  const char maxtmpl[] = "PORT=255.255.255.255:65535\n";
  const char expected[] = "PORT=127.0.0.1:";
  char buf[strlen(maxtmpl)];
  int nread = portfile.ReadAtCurrentPos(buf, sizeof buf);
  if (nread < 0) {
    LOG(ERROR) << "Failed to read Tor control auth cookie\n";
    return false;
  }
  if (static_cast<size_t>(nread) < strlen(mintmpl)) {
    LOG(ERROR) << "Tor control port truncated\n";
    return false;
  }
  CHECK(static_cast<size_t>(nread) <= sizeof buf);

  // Sanity-check the content.
  if (std::string(buf, 0, 5) != "PORT=" || buf[nread - 1] != '\n') {
    LOG(ERROR) << "Invalid Tor control port\n";
    return false;
  }

  // Verify that it's localhost.
  if (std::string(buf, 0, strlen(expected)) != expected) {
    LOG(ERROR) << "Tor control port has non-local control address\n";
    return false;
  }

  // Parse it!
  std::string portstr(buf, strlen(expected), nread - 1);
  if (!base::StringToInt(portstr, &port))
    return false;
  mtime = info.last_modified;
  return true;
}

// Polled()
//
//      Just finished polling the watch directory and failed to
//      establish a connection.  Decide whether to go back to watching
//      and waiting or whether to poll again, if something else
//      happened on the file system while we were busy polling.
//
void TorControl::Polled() {
  switch (state_) {
    case STOPPED:
      // Stopped.  Give up.
      break;
    case WATCHING:
      // Watching and waiting.  Shouldn't happen -- we should either
      // be connecting or connected at this point.
      CHECK(state_ != WATCHING);
      break;
    case CONNECTING:
      // Connecting failed.  Go back to quietly watching and waiting.
      state_ = WATCHING;
      break;
    case CONNECTING_REPOLL:
      // Connecting failed, but while we were waiting, the watcher saw
      // something, so poll again.
      state_ = WATCHING;
      PostPoll();
      break;
    case CONNECTED:
      // We are already connected.  Shouldn't happen -- we only get
      // here if polling or connecting failed.
      CHECK(state_ != CONNECTED);
      break;
  }
}

// OpenControl(portno, cookie)
//
//      Open a control connection on the specified port number at
//      localhost, with the specified control auth cookie.
//
void TorControl::OpenControl(int portno, const std::string& cookie) {
  CHECK(state_ == CONNECTING || state_ == STOPPED);
  if (state_ == STOPPED)
    return;
  net::AddressList addrlist =
    net::AddressList::CreateFromIPAddress(net::IPAddress::IPv4Localhost(),
                                          portno);
  socket_ = std::make_unique<net::TCPClientSocket>(
      addrlist, nullptr, net::NetLog::Get(), net::NetLogSource());
  cookie_ = cookie;
  socket_->Connect(
      base::BindOnce(&TorControl::Connected, base::Unretained(this)));
}

// Connected(error)
//
//      Connection completed.  If it failed, poll again if there was
//      activity while we were busy connecting, or go back to watching
//      and waiting.  If it succeeded, start authenticating.
//
void TorControl::Connected(int error) {
  // We should reach this callback only if we were CONNECTING.  We may
  // have been stopped in the mean time.
  CHECK(state_ == CONNECTING ||
        state_ == CONNECTING_REPOLL ||
        state_ == STOPPED);

  // If we were stopped, disconnect.  Caller will have cancelled the
  // watcher by now.
  if (state_ == STOPPED) {
    socket_.reset();
    return;
  }

  if (error) {
    // Connection failed but there may have been more watch directory
    // activity while we were waiting.  If so, try again; if not, go
    // back to watching and waiting.
    LOG(ERROR) << "Tor control connection failed: "
               << net::ErrorToString(error) << "\n";
    Polled();
    return;
  }

  // Transition from CONNECTING to CONNECTED and send the AUTHENTICATE
  // command.  If the watcher said it's time to repoll, not important
  // any more.
  CHECK(state_ == CONNECTING || state_ == CONNECTING_REPOLL);
  state_ = CONNECTED;
  Cmd1("AUTHENTICATE " + cookie_,
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
  if (error) {
    LOG(ERROR) << "Tor control authentication failed\n";
    Error();
    return;
  }
  for (auto& observer : observers_)
    observer.OnTorReady();
}

// Subscribe(event, callback)
//
//      Subscribe to event by sending SETEVENTS with it included
//      (along with all previously subscribed events).  If repeated,
//      just increment nesting depth without sending SETEVENTS.  Call
//      the callback once the subscription has been processed.
//      Subsequently, whenever the event happens, notify the
//      OnTorAsyncReply observers.
//
void TorControl::Subscribe(TorControlEvent event,
                           base::OnceCallback<void(bool error)> callback) {
  if (async_events_[event]++ == 0) {
    task_runner_->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), true));
    return;
  }

  async_events_[event] = 1;
  SetEvents(
      base::BindOnce(&TorControl::Subscribed,
                     base::Unretained(this), event, std::move(callback)));
}

// Subscribed(event, callback, error, status, reply)
//
//      Internal callback for reply to SETEVENTS from Tor after
//      Subscribe(event, callback).  Cope with failure and call
//      callback.
//
void TorControl::Subscribed(TorControlEvent event,
                            base::OnceCallback<void(bool error)> callback,
                            bool error,
                            const std::string& status,
                            const std::string& reply) {
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
//      will not trigger notification of OnTorAsyncReply observers.
//
void TorControl::Unsubscribe(TorControlEvent event,
                             base::OnceCallback<void(bool error)> callback) {
  // We had better already be subscribed.
  CHECK(async_events_[event] >= 1);
  if (--async_events_[event] > 0) {
    task_runner_->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), true));
    return;
  }

  CHECK(async_events_[event] == 0);
  async_events_.erase(event);
  SetEvents(
      base::BindOnce(&TorControl::Unsubscribed,
                     base::Unretained(this), event, std::move(callback)));
}

// Unsubscribed(event, callback, error, status, reply)
//
//      Internal callback for reply to SETEVENTS from Tor after
//      Unsubscribe(event, callback).  Cope with failure and call
//      callback.
//
void TorControl::Unsubscribed(TorControlEvent event,
                              base::OnceCallback<void(bool error)> callback,
                              bool error,
                              const std::string& status,
                              const std::string& reply) {
  CHECK(async_events_.count(event) == 0);
  if (!error) {
    if (status != "250")
      error = true;
  }
  std::move(callback).Run(error);
}

// SetEvents(callback)
//
//      Send a SETEVENTS command with our current asynchronous event
//      subscriptions, and invoke callback(error) when done.
//
void TorControl::SetEvents(CmdCallback callback) {
  std::ostringstream cmds;
  cmds << "SETEVENTS";
  for (const auto& entry : async_events_) {
    const auto& found = kTorControlEventByEnum.find(entry.first);
    CHECK(found != kTorControlEventByEnum.end());
    cmds << " " << (*found).second;
  }
  Cmd1(cmds.str(), std::move(callback));
}

// Cmd1(cmd, callback)
//
//      Issue a Tor control command for which we only care about the
//      final line; ignore all intermediate lines.
//
void TorControl::Cmd1(const std::string& cmd, CmdCallback callback) {
  Cmd(cmd,
      base::BindRepeating(&TorControl::NullPerLineCallback,
                          base::Unretained(this)),
      std::move(callback));
}

void TorControl::NullPerLineCallback(const std::string& status,
                                     const std::string& reply) {
  // do nothing
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
  for (auto& observer : observers_)
    observer.OnTorCmd(cmd);
  if (writeq_.size() > 100 || cmdq_.size() > 100) {
    // Over 100 commands pending or synchronous callbacks queued --
    // something is probably wrong.
    std::move(callback).Run(/*error=*/true, "", "");
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

// StartWrite()
//
//      Pick a write off the queue and start an I/O buffer for it.
//
//      Caller must ensure writing_ is true.
//
void TorControl::StartWrite() {
  CHECK(writing_);
  CHECK(!writeq_.empty());
  CHECK(!cmdq_.empty());
  auto buf = base::MakeRefCounted<net::StringIOBuffer>(writeq_.front());
  writeiobuf_ = base::MakeRefCounted<net::DrainableIOBuffer>(buf, buf->size());
  writeq_.pop();
}

// DoWrites()
//
//      Issue a write from writeiobuf_, and arrange to issue the rest
//      of the writes in the queue when done.
//
//      Caller must ensure writing_ is true and writeiobuf_ is
//      initialized.
//
void TorControl::DoWrites() {
  CHECK(writing_);
  CHECK(writeiobuf_);
  int rv = socket_->Write(writeiobuf_.get(), writeiobuf_->size(),
                          base::BindOnce(&TorControl::WriteDone,
                                         base::Unretained(this)),
                          tor_control_traffic_annotation);
  if (rv < 0) {
    LOG(ERROR) << "Tor control write error: " << net::ErrorToString(rv)
               << "\n";
    Error();
    return;
  }
}

// WriteDone(rv)
//
//      Callback for write completion.  Advance the write buffer,
//      reissue it if not complete, or if complete pick the next write
//      off the queue and issue it.
//
//      Caller must ensure writing_ is true and writeiobuf_ is
//      initialized.
//
void TorControl::WriteDone(int rv) {
  CHECK(writing_);
  CHECK(writeiobuf_);
  // Bail if error.
  if (rv < 0) {
    LOG(ERROR) << "Tor control write error: " << net::ErrorToString(rv)
               << "\n";
    Error();
    return;
  }
  writeiobuf_->DidConsume(rv);
  // If there's nothing more in the buffer, try to get another off the queue.
  if (!writeiobuf_->BytesRemaining()) {
    // No need to hang on to the I/O buffer any longer.
    writeiobuf_.reset();
    // If there's nothing more in the queue, we're done.
    if (writeq_.empty())
      return;
    // More in the queue.  Start a fresh write.
    StartWrite();
  }
  // Do this write or the next one.
  DoWrites();
}

// StartRead()
//
//      Create an I/O buffer to read command responses into.
//
//      Caller must ensure reading_ is true and that there are
//      synchronous command callbacks or asynchronous event
//      registrations.
//
void TorControl::StartRead() {
  CHECK(reading_);
  CHECK(!cmdq_.empty() || !async_events_.empty());
  readiobuf_ = base::MakeRefCounted<net::GrowableIOBuffer>();
  readiobuf_->SetCapacity(kTorBufferSize);
  read_start_ = 0;
}

// DoReads()
//
//      Issue a read into readiobuf_.
//
//      Caller must ensure reading_ is true and readiobuf_ is
//      initialized.
//
void TorControl::DoReads() {
  CHECK(reading_);
  CHECK(readiobuf_);
  int rv = socket_->Read(readiobuf_.get(), readiobuf_->RemainingCapacity(),
                         base::BindOnce(&TorControl::ReadDone,
                                        base::Unretained(this)));
  if (rv < 0) {
    LOG(ERROR) << "Tor control read error: " << net::ErrorToString(rv)
               << "\n";
    Error();
    return;
  }
}

// ReadDone()
//
//      A read into readiobuf_ just completed.  Process it.
//
void TorControl::ReadDone(int rv) {
  if (rv < 0) {
    LOG(ERROR) << "Tor control read error: " << net::ErrorToString(rv)
               << "\n";
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
        LOG(ERROR) << "Tor control: stray line feed\n";
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
        assert(i >= 2);
        std::string line(readiobuf_->StartOfBuffer() + read_start_,
                         readiobuf_->offset() + i - 2 - read_start_);
        if (!ReadLine(line))
          return;
        read_start_ = readiobuf_->offset() + i;
      } else {
        // CR seen, but not LF.  Bad.
        LOG(ERROR) << "Tor control sent stray carriage return\n";
        Error();
        return;
      }
    }
  }

  // If we've walked up to the end of the buffer, try shifting it to
  // the beginning to make room; if there's no more room, fail --
  // lines shouldn't be this long.
  if (readiobuf_->RemainingCapacity() == 0) {
    if (read_start_ == 0) {
      // Line is too long.
      LOG(ERROR) << "Tor control line too long\n";
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

  // More to do.  Issue the next read.
  DoReads();
}

// ReadLine(line)
//
//      We have read a line of input; process it.  Return true on
//      success, false on error.
//
bool TorControl::ReadLine(const std::string& line) {
  if (line.size() < 4) {
    // Line is too short.
    LOG(ERROR) << "Tor control line too short\n";
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
            LOG(ERROR) << "Ignoring unknown Tor control event: " << event_name
                       << "\n";
            return false;
          }
          const TorControlEvent event = (*found).second;

          // Ignore if we don't think we're subscribed to this.
          if (!async_events_.count(event)) {
            LOG(ERROR) << "Spurious asynchronous Tor event: " << event_name
                       << "\n";
            return true;
          }

          // Notify the observers of the parsed reply.  No extra
          // because there were no intermediate reply lines.
          for (auto& observer : observers_)
            observer.OnTorAsyncReply(event, initial, {});

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
            LOG(ERROR) << "Invalid Tor control async continuation line\n";
            Error();
            return false;
          }
          if (async_->extra.count(key)) {
            LOG(ERROR) << "Duplicate key in Tor control async reply\n";
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
              LOG(ERROR) << "Invalid Tor control async end line\n";
              Error();
              return false;
            }
            if (async_->extra.count(key)) {
              LOG(ERROR) << "Duplicate key in Tor control async reply\n";
              Error();
              return false;
            }
            async_->extra[key] = value;

            // If we're still subscribed, notify the observers of the
            // parsed reply.
            if (async_events_.count(async_->event)) {
              for (auto& observer : observers_)
                observer.OnTorAsyncReply(
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
        LOG(ERROR) << "NYI: data reply on Tor control channel\n";
        Error();
        return false;
      case ' ':
        for (auto& observer : observers_)
          observer.OnTorRawEnd(status, reply);
        if (!cmdq_.empty()) {
          CmdCallback& callback = cmdq_.front().second;
          std::move(callback).Run(/*error=*/false, status, reply);
          cmdq_.pop();
        }
        return true;
    }
  }

  // Not reached if the line is well-formed.
  LOG(ERROR) << "Malformed Tor control reply line\n";
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
  // Invoke all callbacks with errors and clear read state.
  while (!cmdq_.empty()) {
    CmdCallback& callback = cmdq_.front().second;
    std::move(callback).Run(/*error=*/true, "", "");
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

  // Unless we're stopped, go back to watching and poll in case
  // anything happened while we were on the phone to tor.
  if (state_ == STOPPED)
    return;
  state_ = CONNECTING;
  PostPoll();
}

// static
bool TorControl::ParseKV(const std::string& string,
                         std::string& key, std::string& value) {
  size_t end;
  return ParseKV(string, key, value, end) && end == string.size();
}

bool TorControl::ParseKV(const std::string& string,
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

  // Quoted string.
  if (!ParseQuoted(string.substr(eq + 1), value, end))
    return false;
  key = string.substr(0, eq);
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
