// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Plays one Brave Transfer device for interop tests with the Python reference
// peer (tools/brave_transfer/tests/test_cpp_interop.py). Not shipped.
//
// Speaks over stdin/stdout by default, or over TCP with --connect HOST:PORT or
// --listen PORT (which prints "listening PORT" on stdout once bound). Flags
// and the JSON report match the Swift BraveTransferPeer. The run ends like the
// reference peer's: once this side's offer has finished and --expect-offers
// incoming transfers have finished, it closes its output and reads until the
// input reaches EOF.

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "base/at_exit.h"
#include "base/compiler_specific.h"
#include "base/containers/extend.h"
#include "base/containers/span.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/types/expected_macros.h"
#include "base/values.h"
#include "brave/components/brave_transfer/core/filename_sanitizer.h"
#include "brave/components/brave_transfer/core/frame_codec.h"
#include "brave/components/brave_transfer/core/secure_channel.h"
#include "brave/components/brave_transfer/core/session.h"
#include "brave/components/brave_transfer/core/trust_keys.h"
#include "crypto/hash.h"

namespace brave_transfer {
namespace {

constexpr char kUsage[] =
    "usage: brave_transfer_peer --role initiator|responder --seed HEX "
    "[options]\n"
    "  --static HEX           static X25519 private key (random by default)\n"
    "  --name NAME            device name\n"
    "  --guid GUID            sync device GUID\n"
    "  --allow-guid GUID      only talk to these remote GUIDs (repeatable)\n"
    "  --send-text TEXT       offer a text item (repeatable)\n"
    "  --send-url URL         offer a URL item (repeatable)\n"
    "  --send-file PATH       offer a file (repeatable)\n"
    "  --accept all|first|decline\n"
    "  --expect-offers N      incoming transfers to wait for\n"
    "  --receive-dir DIR      where received items are written\n"
    "  --report PATH          write a JSON report here\n"
    "  --connect HOST:PORT    use TCP, dialing HOST:PORT\n"
    "  --listen PORT          use TCP, accepting one connection on 127.0.0.1\n";

constexpr size_t kMaxBufferedOutput = 1 << 18;

void Log(std::string_view message) {
  base::WriteFileDescriptor(
      STDERR_FILENO, "brave_transfer_peer: " + std::string(message) + "\n");
}

[[noreturn]] void Fail(std::string_view message) {
  Log(message);
  _exit(1);
}

struct Options {
  bool initiator = true;
  Key seed{};
  std::optional<Key> static_key;
  std::string name = "C++ peer";
  std::string guid;
  std::set<std::string> allowed_guids;
  std::vector<std::string> texts;
  std::vector<std::string> urls;
  std::vector<std::string> files;
  std::string accept = "all";
  int expect_offers = 0;
  std::optional<base::FilePath> receive_dir;
  std::optional<base::FilePath> report;
  std::optional<std::pair<std::string, std::string>> connect;
  std::optional<int> listen_port;
};

std::optional<Key> KeyFromHex(std::string_view hex) {
  Key key;
  if (!base::HexStringToSpan(hex, key)) {
    return std::nullopt;
  }
  return key;
}

Options ParseOptions(base::span<char*> argv) {
  std::vector<std::string> args;
  for (const char* arg : argv.subspan(1u)) {
    args.emplace_back(arg);
  }
  Options options;
  std::optional<std::string> role;
  for (size_t i = 0; i < args.size(); ++i) {
    const std::string& flag = args[i];
    auto value = [&]() -> std::string {
      if (i + 1 >= args.size()) {
        Fail(flag + " needs a value\n" + kUsage);
      }
      return args[++i];
    };
    if (flag == "--role") {
      role = value();
    } else if (flag == "--seed") {
      std::optional<Key> seed = KeyFromHex(value());
      if (!seed) {
        Fail("--seed must be 32 bytes of hex");
      }
      options.seed = *seed;
    } else if (flag == "--static") {
      options.static_key = KeyFromHex(value());
      if (!options.static_key) {
        Fail("--static must be 32 bytes of hex");
      }
    } else if (flag == "--name") {
      options.name = value();
    } else if (flag == "--guid") {
      options.guid = value();
    } else if (flag == "--allow-guid") {
      options.allowed_guids.insert(value());
    } else if (flag == "--send-text") {
      options.texts.push_back(value());
    } else if (flag == "--send-url") {
      options.urls.push_back(value());
    } else if (flag == "--send-file") {
      options.files.push_back(value());
    } else if (flag == "--accept") {
      options.accept = value();
    } else if (flag == "--expect-offers") {
      if (!base::StringToInt(value(), &options.expect_offers)) {
        Fail("--expect-offers needs a number");
      }
    } else if (flag == "--receive-dir") {
      options.receive_dir = base::FilePath(value());
    } else if (flag == "--report") {
      options.report = base::FilePath(value());
    } else if (flag == "--connect") {
      std::string target = value();
      size_t colon = target.rfind(':');
      if (colon == std::string::npos) {
        Fail("--connect needs HOST:PORT");
      }
      options.connect.emplace(target.substr(0, colon),
                              target.substr(colon + 1));
    } else if (flag == "--listen") {
      int port = 0;
      if (!base::StringToInt(value(), &port) || port < 0 || port > 65535) {
        Fail("--listen needs a port");
      }
      options.listen_port = port;
    } else {
      Fail("unknown argument " + flag + "\n" + kUsage);
    }
  }
  if (role != "initiator" && role != "responder") {
    Fail(kUsage);
  }
  options.initiator = *role == "initiator";
  if (options.accept != "all" && options.accept != "first" &&
      options.accept != "decline") {
    Fail(kUsage);
  }
  return options;
}

int ConnectTcp(const std::string& host, const std::string& port) {
  addrinfo hints = {};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo* addresses = nullptr;
  if (getaddrinfo(host.c_str(), port.c_str(), &hints, &addresses) != 0) {
    Fail("cannot resolve " + host);
  }
  int fd = -1;
  for (addrinfo* a = addresses; a; a = a->ai_next) {
    fd = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
    if (fd >= 0 && connect(fd, a->ai_addr, a->ai_addrlen) == 0) {
      break;
    }
    if (fd >= 0) {
      close(fd);
      fd = -1;
    }
  }
  freeaddrinfo(addresses);
  if (fd < 0) {
    Fail("cannot connect to " + host + ":" + port);
  }
  return fd;
}

int AcceptTcp(int port) {
  int listener = socket(AF_INET, SOCK_STREAM, 0);
  if (listener < 0) {
    Fail("socket failed");
  }
  int reuse = 1;
  setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  sockaddr_in address = {};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  address.sin_port = htons(static_cast<uint16_t>(port));
  if (bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) !=
          0 ||
      listen(listener, 1) != 0) {
    Fail("cannot listen on port " + base::NumberToString(port));
  }
  socklen_t length = sizeof(address);
  getsockname(listener, reinterpret_cast<sockaddr*>(&address), &length);
  base::WriteFileDescriptor(
      STDOUT_FILENO,
      "listening " + base::NumberToString(ntohs(address.sin_port)) + "\n");
  int fd = accept(listener, nullptr, nullptr);
  close(listener);
  if (fd < 0) {
    Fail("accept failed");
  }
  return fd;
}

// Non-blocking byte stream with length-prefixed message framing.
class Transport {
 public:
  Transport(int in_fd, int out_fd) : in_fd_(in_fd), out_fd_(out_fd) {
    fcntl(out_fd_, F_SETFL, fcntl(out_fd_, F_GETFL) | O_NONBLOCK);
  }

  void Send(base::span<const uint8_t> bytes) { base::Extend(out_, bytes); }

  void CloseOutput() {
    if (in_fd_ == out_fd_) {
      shutdown(out_fd_, SHUT_WR);
    } else {
      close(out_fd_);
    }
    output_closed_ = true;
  }

  std::vector<Bytes> TakeMessages() { return std::exchange(inbox_, {}); }

  // Waits up to `timeout_ms` for input to be readable or output writable.
  Result<void> Pump(int timeout_ms) {
    std::vector<pollfd> fds;
    if (!input_closed_) {
      fds.push_back({in_fd_, POLLIN, 0});
    }
    if (!out_.empty() && !output_closed_) {
      if (in_fd_ == out_fd_ && !fds.empty()) {
        fds[0].events |= POLLOUT;
      } else {
        fds.push_back({out_fd_, POLLOUT, 0});
      }
    }
    if (fds.empty()) {
      return base::ok();
    }
    if (poll(fds.data(), fds.size(), timeout_ms) < 0) {
      if (errno == EINTR) {
        return base::ok();
      }
      return base::unexpected("poll failed");
    }
    for (const pollfd& fd : fds) {
      if (fd.fd == in_fd_ && !input_closed_ &&
          (fd.revents & (POLLIN | POLLHUP | POLLERR))) {
        RETURN_IF_ERROR(ReadInput());
      }
      if (fd.fd == out_fd_ && (fd.revents & (POLLOUT | POLLERR)) &&
          !out_.empty() && !output_closed_) {
        RETURN_IF_ERROR(WriteOutput());
      }
    }
    return base::ok();
  }

  Result<void> Flush() {
    while (!out_.empty()) {
      RETURN_IF_ERROR(Pump(-1));
    }
    return base::ok();
  }

  Result<Bytes> ReadMessage() {
    while (inbox_.empty()) {
      if (input_closed_) {
        return base::unexpected("connection closed during handshake");
      }
      RETURN_IF_ERROR(Pump(-1));
    }
    Bytes message = std::move(inbox_.front());
    inbox_.erase(inbox_.begin());
    return message;
  }

  size_t buffered_output() const { return out_.size(); }
  bool input_closed() const { return input_closed_; }
  bool output_closed() const { return output_closed_; }

 private:
  Result<void> ReadInput() {
    Bytes buffer(1 << 16);
    ssize_t n = read(in_fd_, buffer.data(), buffer.size());
    if (n > 0) {
      for (Bytes& message :
           reader_.Feed(base::span(buffer).first(static_cast<size_t>(n)))) {
        inbox_.push_back(std::move(message));
      }
    } else if (n == 0) {
      input_closed_ = true;
      if (reader_.HasPartialMessage()) {
        return base::unexpected("connection closed mid-message");
      }
    } else if (errno != EAGAIN && errno != EINTR) {
      return base::unexpected("read failed");
    }
    return base::ok();
  }

  Result<void> WriteOutput() {
    ssize_t n = write(out_fd_, out_.data(), out_.size());
    if (n > 0) {
      out_.erase(out_.begin(), out_.begin() + n);
    } else if (n < 0 && errno != EAGAIN && errno != EINTR) {
      return base::unexpected("write failed");
    }
    return base::ok();
  }

  const int in_fd_;
  const int out_fd_;
  MessageReader reader_;
  std::vector<Bytes> inbox_;
  Bytes out_;
  bool input_closed_ = false;
  bool output_closed_ = false;
};

std::optional<Bytes> ReadFileRange(const base::FilePath& path,
                                   uint64_t offset,
                                   size_t length) {
  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  Bytes data(length);
  if (!file.IsValid() ||
      file.Read(static_cast<int64_t>(offset), data) != length) {
    return std::nullopt;
  }
  return data;
}

Result<OutgoingItem> FileItem(const std::string& path_string) {
  base::FilePath path(path_string);
  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!file.IsValid()) {
    return base::unexpected("cannot open " + path_string);
  }
  crypto::hash::Hasher hasher(crypto::hash::kSha256);
  Bytes block(1 << 20);
  uint64_t size = 0;
  while (true) {
    std::optional<size_t> n = file.ReadAtCurrentPos(block);
    if (!n) {
      return base::unexpected("cannot read " + path_string);
    }
    if (*n == 0) {
      break;
    }
    hasher.Update(base::span(block).first(*n));
    size += *n;
  }
  OutgoingItem item;
  item.kind = proto::ITEM_KIND_FILE;
  item.name = path.BaseName().value();
  item.mime_type = "application/octet-stream";
  item.size = size;
  item.sha256.resize(crypto::hash::kSha256Size);
  hasher.Finish(item.sha256);
  item.read = base::BindRepeating(&ReadFileRange, path);
  return item;
}

std::string TransferKey(const std::string& transfer_id, uint32_t item_id) {
  return base::HexEncodeLower(transfer_id) + "/" +
         base::NumberToString(item_id);
}

base::Value OptionalEnum(std::optional<int> value) {
  return value ? base::Value(*value) : base::Value();
}

// Writes received items and records what happened, in the same JSON shape as
// the Python reference peer's report.
class Receiver {
 public:
  explicit Receiver(std::optional<base::FilePath> directory)
      : directory_(std::move(directory)) {
    if (directory_) {
      base::CreateDirectory(*directory_);
    }
  }

  void OfferReceived(const proto::Offer& offer) {
    for (const proto::OfferItem& item : offer.items()) {
      names_[TransferKey(offer.transfer_id(), item.item_id())] = item.name();
    }
  }

  Result<void> SaveWhole(const std::string& transfer_id,
                         const proto::OfferItem& item,
                         base::span<const uint8_t> data) {
    std::optional<std::string> inline_text;
    if (item.kind() == proto::ITEM_KIND_TEXT ||
        item.kind() == proto::ITEM_KIND_URL) {
      inline_text = std::string(base::as_string_view(data));
    }
    std::optional<base::FilePath> saved;
    if (directory_) {
      saved = UniquePath(SanitizeFilename(item.name()));
      if (!base::WriteFile(*saved, data)) {
        return base::unexpected("cannot write " + saved->value());
      }
    }
    Record(transfer_id, item, saved, inline_text);
    return base::ok();
  }

  Result<void> WriteChunk(const std::string& transfer_id,
                          uint32_t item_id,
                          std::string_view data) {
    if (!directory_) {
      return base::ok();
    }
    const std::string key = TransferKey(transfer_id, item_id);
    auto it = open_files_.find(key);
    if (it == open_files_.end()) {
      OpenFile open;
      open.final_path = UniquePath(SanitizeFilename(names_[key]));
      open.part_path = open.final_path.AddExtensionASCII("part");
      open.file = base::File(open.part_path, base::File::FLAG_CREATE_ALWAYS |
                                                 base::File::FLAG_WRITE);
      if (!open.file.IsValid()) {
        return base::unexpected("cannot create " + open.part_path.value());
      }
      it = open_files_.emplace(key, std::move(open)).first;
    }
    if (!it->second.file.WriteAtCurrentPosAndCheck(base::as_byte_span(data))) {
      return base::unexpected("write failed");
    }
    return base::ok();
  }

  Result<void> FinishItem(const std::string& transfer_id,
                          const proto::OfferItem& item) {
    auto it = open_files_.find(TransferKey(transfer_id, item.item_id()));
    if (it != open_files_.end()) {
      OpenFile open = std::move(it->second);
      open_files_.erase(it);
      open.file.Close();
      if (!base::Move(open.part_path, open.final_path)) {
        return base::unexpected("cannot rename " + open.part_path.value());
      }
      Record(transfer_id, item, open.final_path, std::nullopt);
    } else if (item.size() == 0) {
      return SaveWhole(transfer_id, item, {});
    } else {
      Record(transfer_id, item, std::nullopt, std::nullopt);
    }
    return base::ok();
  }

  void TransferFinished(const TransferResult& result) {
    const std::string prefix = base::HexEncodeLower(result.transfer_id) + "/";
    for (auto it = open_files_.begin(); it != open_files_.end();) {
      if (it->first.starts_with(prefix)) {
        it->second.file.Close();
        base::DeleteFile(it->second.part_path);
        it = open_files_.erase(it);
      } else {
        ++it;
      }
    }
    finished_.Append(
        base::DictValue()
            .Set("transfer_id", base::HexEncodeLower(result.transfer_id))
            .Set("outgoing", result.is_outgoing)
            .Set("status", OptionalEnum(result.status))
            .Set("cancel_reason", OptionalEnum(result.cancel_reason))
            .Set("decision", OptionalEnum(result.decision)));
  }

  base::ListValue TakeReceived() { return std::move(received_); }
  base::ListValue TakeFinished() { return std::move(finished_); }

 private:
  struct OpenFile {
    base::FilePath final_path;
    base::FilePath part_path;
    base::File file;
  };

  base::FilePath UniquePath(const std::string& name) {
    const size_t dot = name.find('.');
    const std::string stem = name.substr(0, dot);
    const std::string extension =
        dot == std::string::npos ? std::string() : name.substr(dot);
    base::FilePath candidate = directory_->Append(name);
    for (int n = 1; base::PathExists(candidate) ||
                    base::PathExists(candidate.AddExtensionASCII("part"));
         ++n) {
      candidate = directory_->Append(stem + " (" + base::NumberToString(n) +
                                     ")" + extension);
    }
    return candidate;
  }

  void Record(const std::string& transfer_id,
              const proto::OfferItem& item,
              const std::optional<base::FilePath>& saved_as,
              const std::optional<std::string>& inline_text) {
    received_.Append(
        base::DictValue()
            .Set("transfer_id", base::HexEncodeLower(transfer_id))
            .Set("item_id", static_cast<int>(item.item_id()))
            .Set("kind", static_cast<int>(item.kind()))
            .Set("name", item.name())
            .Set("size", static_cast<double>(item.size()))
            .Set("sha256", base::HexEncodeLower(item.sha256()))
            .Set("saved_as",
                 saved_as ? base::Value(saved_as->value()) : base::Value())
            .Set("inline_data",
                 inline_text ? base::Value(*inline_text) : base::Value()));
  }

  const std::optional<base::FilePath> directory_;
  std::map<std::string, OpenFile> open_files_;
  std::map<std::string, std::string> names_;
  base::ListValue received_;
  base::ListValue finished_;
};

class Peer {
 public:
  Peer(const Options& options, Transport& io)
      : options_(options), io_(io), receiver_(options.receive_dir) {}

  Result<void> Run() {
    const ChainKeys keys = *ChainKeys::FromSeed(options_.seed);
    const Key private_key =
        options_.static_key.value_or(static_key::GeneratePrivateKey());
    Log("local fingerprint: " +
        static_key::Fingerprint(static_key::PublicKey(private_key)));

    proto::PeerInfo info;
    info.set_protocol_version(kProtocolVersion);
    info.set_device_name(options_.name);
    info.set_device_type(options_.initiator ? proto::DEVICE_TYPE_DESKTOP
                                            : proto::DEVICE_TYPE_PHONE);
    info.set_sync_device_guid(options_.guid);
    info.set_capabilities(kCapabilityReceive | kCapabilitySend);

    std::vector<OutgoingItem> items;
    for (const std::string& text : options_.texts) {
      items.push_back(OutgoingItem::FromBytes(proto::ITEM_KIND_TEXT, "text.txt",
                                              "text/plain",
                                              Bytes(text.begin(), text.end())));
    }
    for (const std::string& url : options_.urls) {
      items.push_back(OutgoingItem::FromBytes(proto::ITEM_KIND_URL, url,
                                              "text/uri-list",
                                              Bytes(url.begin(), url.end())));
    }
    for (const std::string& file : options_.files) {
      ASSIGN_OR_RETURN(OutgoingItem item, FileItem(file));
      items.push_back(std::move(item));
    }

    std::unique_ptr<SecureChannel> channel;
    if (options_.initiator) {
      const base::Time now = base::Time::Now();
      ASSIGN_OR_RETURN(auto handshake,
                       InitiatorHandshake::Create(
                           private_key, keys.psk(),
                           keys.TagForEpoch(EpochForTime(now)), info));
      ASSIGN_OR_RETURN(Bytes m1, handshake->Message1());
      RETURN_IF_ERROR(SendPrefixed(m1));
      ASSIGN_OR_RETURN(Bytes m2, io_->ReadMessage());
      RETURN_IF_ERROR(handshake->ReadMessage2(m2));
      ASSIGN_OR_RETURN(Bytes m3, handshake->Message3());
      RETURN_IF_ERROR(SendPrefixed(m3));
      ASSIGN_OR_RETURN(channel, handshake->TakeChannel());
    } else {
      ResponderHandshake handshake(
          private_key, info,
          ResponderHandshake::PskForChain(keys, base::Time::Now()));
      ASSIGN_OR_RETURN(Bytes m1, io_->ReadMessage());
      RETURN_IF_ERROR(handshake.ReadMessage1(m1));
      ASSIGN_OR_RETURN(Bytes m2, handshake.Message2());
      RETURN_IF_ERROR(SendPrefixed(m2));
      ASSIGN_OR_RETURN(Bytes m3, io_->ReadMessage());
      RETURN_IF_ERROR(handshake.ReadMessage3(m3));
      ASSIGN_OR_RETURN(channel, handshake.TakeChannel());
    }
    if (!options_.allowed_guids.empty() &&
        !options_.allowed_guids.contains(
            channel->remote_info().sync_device_guid())) {
      return base::unexpected("remote device is not in the sync chain");
    }
    Log("remote fingerprint: " +
        static_key::Fingerprint(channel->remote_static_key()));

    TransferSession session(base::TimeTicks::Now(), ReceiveLimits(),
                            /*send_confirmation=*/!options_.initiator);
    session_ = &session;
    outgoing_done_ = items.empty();
    bool offer_sent = false;

    while (true) {
      if (channel->is_confirmed() && !io_->output_closed()) {
        if (!offer_sent && !items.empty()) {
          RETURN_IF_ERROR(
              session.SendOffer(std::move(items), base::TimeTicks::Now()));
          offer_sent = true;
        }
        while (io_->buffered_output() < kMaxBufferedOutput) {
          ASSIGN_OR_RETURN(std::optional<proto::Frame> frame,
                           session.NextFrame(base::TimeTicks::Now()));
          if (!frame) {
            break;
          }
          ASSIGN_OR_RETURN(Bytes wire, channel->Seal(*frame));
          io_->Send(wire);
        }
        if (WorkDone() && io_->buffered_output() == 0) {
          io_->CloseOutput();
        }
      }
      if (io_->input_closed()) {
        if (io_->output_closed()) {
          break;
        }
        if (!WorkDone()) {
          return base::unexpected("remote closed before we finished");
        }
      }
      RETURN_IF_ERROR(io_->Pump(1000));
      for (const Bytes& message : io_->TakeMessages()) {
        ASSIGN_OR_RETURN(proto::Frame frame, channel->Open(message));
        ASSIGN_OR_RETURN(TransferEvents events,
                         session.Handle(frame, base::TimeTicks::Now()));
        RETURN_IF_ERROR(HandleEvents(std::move(events)));
      }
      if (!io_->output_closed()) {
        ASSIGN_OR_RETURN(TransferEvents events,
                         session.Poll(base::TimeTicks::Now()));
        RETURN_IF_ERROR(HandleEvents(std::move(events)));
      }
    }
    session_ = nullptr;

    if (options_.report) {
      const proto::PeerInfo& remote = channel->remote_info();
      base::DictValue report =
          base::DictValue()
              .Set("remote_info",
                   base::DictValue()
                       .Set("protocol_version",
                            static_cast<int>(remote.protocol_version()))
                       .Set("device_name", remote.device_name())
                       .Set("device_type",
                            static_cast<int>(remote.device_type()))
                       .Set("sync_device_guid", remote.sync_device_guid())
                       .Set("capabilities",
                            static_cast<double>(remote.capabilities())))
              .Set("remote_fingerprint",
                   static_key::Fingerprint(channel->remote_static_key()))
              .Set("handshake_hash",
                   base::HexEncodeLower(channel->handshake_hash()))
              .Set("received", receiver_.TakeReceived())
              .Set("finished", receiver_.TakeFinished());
      std::optional<std::string> json = base::WriteJsonWithOptions(
          report, base::JSONWriter::OPTIONS_PRETTY_PRINT);
      if (!json || !base::WriteFile(*options_.report, *json)) {
        return base::unexpected("cannot write the report");
      }
    }
    return base::ok();
  }

 private:
  bool WorkDone() const {
    return outgoing_done_ && incoming_finished_ >= options_.expect_offers;
  }

  Result<void> SendPrefixed(base::span<const uint8_t> message) {
    ASSIGN_OR_RETURN(Bytes prefixed, LengthPrefixed(message));
    io_->Send(prefixed);
    return io_->Flush();
  }

  Result<void> HandleEvents(TransferEvents events) {
    for (TransferEvent& event : events) {
      if (auto* offer = std::get_if<OfferReceived>(&event)) {
        receiver_.OfferReceived(offer->offer);
        proto::Decision decision = proto::DECISION_DECLINE;
        std::vector<uint32_t> ids;
        if (options_.accept == "all") {
          decision = proto::DECISION_ACCEPT_ALL;
        } else if (options_.accept == "first") {
          decision = proto::DECISION_ACCEPT_SOME;
          ids.push_back(offer->offer.items(0).item_id());
        }
        ASSIGN_OR_RETURN(
            TransferEvents more,
            session_->Respond(offer->offer.transfer_id(), decision, ids));
        RETURN_IF_ERROR(HandleEvents(std::move(more)));
      } else if (auto* inline_item = std::get_if<InlineItemReceived>(&event)) {
        RETURN_IF_ERROR(receiver_.SaveWhole(
            inline_item->transfer_id, inline_item->item,
            base::as_byte_span(inline_item->item.inline_data())));
      } else if (auto* chunk = std::get_if<ChunkReceived>(&event)) {
        RETURN_IF_ERROR(receiver_.WriteChunk(chunk->transfer_id, chunk->item_id,
                                             chunk->data));
      } else if (auto* item = std::get_if<ItemReceived>(&event)) {
        RETURN_IF_ERROR(receiver_.FinishItem(item->transfer_id, item->item));
      } else if (auto* finished =
                     std::get_if<brave_transfer::TransferFinished>(&event)) {
        receiver_.TransferFinished(finished->result);
        if (finished->result.is_outgoing) {
          outgoing_done_ = true;
        } else {
          ++incoming_finished_;
        }
      }
    }
    return base::ok();
  }

  const Options options_;
  const raw_ref<Transport> io_;
  Receiver receiver_;
  raw_ptr<TransferSession> session_ = nullptr;
  bool outgoing_done_ = false;
  int incoming_finished_ = 0;
};

}  // namespace
}  // namespace brave_transfer

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  signal(SIGPIPE, SIG_IGN);
  // SAFETY: main() receives `argc` entries in `argv`.
  brave_transfer::Options options = brave_transfer::ParseOptions(
      UNSAFE_BUFFERS(base::span(argv, static_cast<size_t>(argc))));
  int in_fd = STDIN_FILENO;
  int out_fd = STDOUT_FILENO;
  if (options.connect) {
    in_fd = out_fd = brave_transfer::ConnectTcp(options.connect->first,
                                                options.connect->second);
  } else if (options.listen_port) {
    in_fd = out_fd = brave_transfer::AcceptTcp(*options.listen_port);
  }
  brave_transfer::Transport io(in_fd, out_fd);
  brave_transfer::Peer peer(options, io);
  brave_transfer::Result<void> result = peer.Run();
  if (!result.has_value()) {
    brave_transfer::Fail(result.error());
  }
  return 0;
}
