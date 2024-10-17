/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "brave/components/tor/tor_file_watcher.h"

#include <string>
#include <utility>

#include "base/files/file.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "build/build_config.h"

namespace tor {

namespace {

constexpr base::TaskTraits kWatchTaskTraits = {base::MayBlock(),
                                               base::TaskPriority::BEST_EFFORT};

#if BUILDFLAG(IS_WIN)
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
}  // namespace

TorFileWatcher::TorFileWatcher(const base::FilePath& watch_dir_path)
    : polling_(false),
      repoll_(false),
      watch_dir_path_(std::move(watch_dir_path)),
      watch_task_runner_(
          base::ThreadPool::CreateSequencedTaskRunner(kWatchTaskTraits)),
      watcher_(new base::FilePathWatcher,
               base::OnTaskRunnerDeleter(watch_task_runner_)) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owner_sequence_checker_);
  DETACH_FROM_SEQUENCE(watch_sequence_checker_);
}

TorFileWatcher::~TorFileWatcher() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watch_sequence_checker_);
}

void TorFileWatcher::StartWatching(WatchCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owner_sequence_checker_);
  watch_callback_ = std::move(callback);
  watch_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&TorFileWatcher::StartWatchingOnTaskRunner,
                                weak_ptr_factory_.GetWeakPtr()));
}

void TorFileWatcher::StartWatchingOnTaskRunner() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watch_sequence_checker_);
  if (!watcher_->Watch(watch_dir_path_,
                       base::FilePathWatcher::Type::kNonRecursive,
                       base::BindRepeating(&TorFileWatcher::OnWatchDirChanged,
                                           weak_ptr_factory_.GetWeakPtr()))) {
    // Never mind -- destroy the watcher and stop everything else.
    VLOG(0) << "tor: failed to watch directory";
    OnWatchDirChanged(base::FilePath(), true);
    return;
  }
  polling_ = true;
  Poll();
}

// Watching for startup

// WatchDirChanged(path, error)
//
//      Something happened in the watch directory at path.  If we're
//      already polling, make sure to try again if it fails -- the tor
//      daemon may now be ready if it wasn't before.  Otherwise, start
//      polling.
//
void TorFileWatcher::OnWatchDirChanged(const base::FilePath& path, bool error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watch_sequence_checker_);
  VLOG(2) << "tor: watch directory changed";

  if (error) {
    std::move(watch_callback_).Run(false, std::vector<uint8_t>(), int());
    if (!watch_task_runner_->DeleteSoon(FROM_HERE, this))
      delete this;
    return;
  }

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
void TorFileWatcher::Poll() {
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

  std::move(watch_callback_).Run(true, std::move(cookie), port);
  if (!watch_task_runner_->DeleteSoon(FROM_HERE, this))
    delete this;
}

// PollDone()
//
//      Just finished polling the watch directory and failed to
//      establish a connection.  Decide whether to go back to watching
//      and waiting or whether to poll again, if something else
//      happened on the file system while we were busy polling.
//
void TorFileWatcher::PollDone() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(watch_sequence_checker_);
  DCHECK(polling_);

  if (repoll_) {
    VLOG(2) << "tor: retrying control connection";
    repoll_ = false;
    watch_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&TorFileWatcher::Poll, weak_ptr_factory_.GetWeakPtr()));
  } else {
    VLOG(2) << "tor: control connection not yet ready";
    polling_ = false;
  }
}

// EatControlCookie(cookie, mtime)
//
//      Try to read the control auth cookie.  Return true and set
//      cookie and mtime if successful; return false on failure.
//
bool TorFileWatcher::EatControlCookie(std::vector<uint8_t>& cookie,
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
  if (nread <= 0) {
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
bool TorFileWatcher::EatControlPort(int& port, base::Time& mtime) {
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
  const size_t kBufSiz = sizeof(kControlPortMaxTmpl);
  char buf[kBufSiz];
  int nread = portfile.ReadAtCurrentPos(buf, sizeof buf);
  if (nread < 0) {
    VLOG(0) << "tor: failed to read control port";
    return false;
  } else if (static_cast<size_t>(nread) >= sizeof buf) {
    VLOG(0) << "tor: control port too long";
    return false;
  }

  if (static_cast<size_t>(nread) < strlen(kControlPortMinTmpl)) {
    VLOG(0) << "tor: control port truncated";
    return false;
  }

  buf[nread] = '\0';
  std::string text(buf);

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
  if (port > 65535) {
    VLOG(0) << "tor: port overflow";
    return false;
  }
  mtime = info.last_modified;
  VLOG(3) << "Control port " << port << ", mtime " << mtime;
  return true;
}

}  // namespace tor
