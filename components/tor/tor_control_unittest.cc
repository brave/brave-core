/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_control.h"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/barrier_closure.h"
#include "base/compiler_specific.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/run_until.h"
#include "base/test/test_future.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/browser_task_environment.h"
#include "net/base/io_buffer.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/base/test_completion_callback.h"
#include "net/log/net_log.h"
#include "net/log/net_log_event_type.h"
#include "net/log/net_log_source.h"
#include "net/log/test_net_log.h"
#include "net/log/test_net_log_util.h"
#include "net/socket/stream_socket.h"
#include "net/socket/tcp_server_socket.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace tor {
namespace {
class MockTorControlDelegate : public TorControl::Delegate {
 public:
  MOCK_METHOD0(OnTorControlReady, void());
  MOCK_METHOD1(OnTorControlClosed, void(bool));
  MOCK_METHOD3(OnTorEvent,
               void(TorControlEvent,
                    const std::string&,
                    const std::map<std::string, std::string>&));
  MOCK_METHOD1(OnTorRawCmd, void(const std::string&));
  MOCK_METHOD2(OnTorRawAsync, void(const std::string&, const std::string&));
  MOCK_METHOD2(OnTorRawMid, void(const std::string&, const std::string&));
  MOCK_METHOD2(OnTorRawEnd, void(const std::string&, const std::string&));

  base::WeakPtr<tor::TorControl::Delegate> AsWeakPtr() override {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  base::WeakPtrFactory<tor::TorControl::Delegate> weak_ptr_factory_{this};
};

// Drives a TorControl through its real read path: TorControl connects to a
// loopback TCPServerSocket owned by the test, which plays the Tor process.
// What TorControl parsed is observed through the delegate.
class TorControlReadTest : public testing::Test {
 public:
  TorControlReadTest()
      : task_environment_(content::BrowserTaskEnvironment::IO_MAINLOOP),
        io_task_runner_(content::GetIOThreadTaskRunner({})),
        control_(new TorControl(delegate_.AsWeakPtr(), io_task_runner_),
                 base::OnTaskRunnerDeleter(io_task_runner_)) {}

  void SetUp() override {
    server_ = std::make_unique<net::TCPServerSocket>(net::NetLog::Get(),
                                                     net::NetLogSource());
    ASSERT_EQ(net::OK, server_->Listen(
                           net::IPEndPoint(net::IPAddress::IPv4Localhost(), 0),
                           1, std::nullopt));
    net::IPEndPoint address;
    ASSERT_EQ(net::OK, server_->GetLocalAddress(&address));
    net::RecordingNetLogObserver connections;
    ASSERT_NO_FATAL_FAILURE(Handshake(address.port()));
    for (const auto& entry :
         connections.GetEntriesWithType(net::NetLogEventType::TCP_CONNECT)) {
      const auto* peer = entry.params.FindString("remote_address");
      if (peer && *peer == address.ToString()) {
        control_socket_source_ = entry.source;
        break;
      }
    }
  }

 protected:
  // Connects, authenticates and subscribes to an event, so TorControl keeps
  // reading after every command has been answered.
  void Handshake(int port) {
    base::RunLoop ready_loop;
    base::RunLoop replies_loop;
    // The mock keeps this closure beyond an early return from this function,
    // so it is captured by value: copies share the barrier's ref-counted state.
    base::RepeatingClosure replied =
        base::BarrierClosure(4, replies_loop.QuitClosure());
    EXPECT_CALL(delegate_, OnTorControlReady())
        .WillOnce(base::test::RunClosure(ready_loop.QuitClosure()));
    EXPECT_CALL(delegate_, OnTorRawEnd("250", "OK"))
        .Times(4)
        .WillRepeatedly([replied] { replied.Run(); });

    control_->Start({0xAB, 0xCD}, port);
    net::TestCompletionCallback accept_callback;
    ASSERT_EQ(net::OK, accept_callback.GetResult(server_->Accept(
                           &client_, accept_callback.callback())));

    ASSERT_NO_FATAL_FAILURE(ExpectCommand("AUTHENTICATE ABCD"));
    ASSERT_NO_FATAL_FAILURE(Send("250 OK\r\n"));
    ready_loop.Run();
    ASSERT_NO_FATAL_FAILURE(ExpectCommand("TAKEOWNERSHIP"));
    ASSERT_NO_FATAL_FAILURE(Send("250 OK\r\n"));
    ASSERT_NO_FATAL_FAILURE(
        ExpectCommand("RESETCONF __OwningControllerProcess"));
    ASSERT_NO_FATAL_FAILURE(Send("250 OK\r\n"));

    ASSERT_NO_FATAL_FAILURE(Subscribe(TorControlEvent::NETWORK_LIVENESS,
                                      "SETEVENTS NETWORK_LIVENESS"));
    replies_loop.Run();
    testing::Mock::VerifyAndClearExpectations(&delegate_);
  }

  // Subscribes to `event` and answers the SETEVENTS command, which lists every
  // subscribed event.
  void Subscribe(TorControlEvent event, std::string_view expected_command) {
    base::test::TestFuture<bool> subscribed;
    control_->Subscribe(event, subscribed.GetCallback());
    ASSERT_NO_FATAL_FAILURE(ExpectCommand(expected_command));
    ASSERT_NO_FATAL_FAILURE(Send("250 OK\r\n"));
    EXPECT_FALSE(subscribed.Get());
  }

  // Reads one CRLF-terminated command TorControl wrote to the socket.
  void ExpectCommand(std::string_view expected) {
    while (pending_.find("\r\n") == std::string::npos) {
      auto buffer = base::MakeRefCounted<net::IOBufferWithSize>(1024);
      net::TestCompletionCallback callback;
      int rv = callback.GetResult(
          client_->Read(buffer.get(), buffer->size(), callback.callback()));
      ASSERT_GT(rv, 0);
      pending_.append(buffer->data(), static_cast<size_t>(rv));
    }
    size_t end = pending_.find("\r\n");
    EXPECT_EQ(pending_.substr(0, end), expected);
    pending_.erase(0, end + 2);
  }

  // Writes raw bytes to TorControl, as the Tor process would.
  void Send(std::string_view data) {
    auto buffer = base::MakeRefCounted<net::DrainableIOBuffer>(
        base::MakeRefCounted<net::StringIOBuffer>(std::string(data)),
        data.size());
    while (buffer->BytesRemaining() > 0) {
      net::TestCompletionCallback callback;
      int rv = callback.GetResult(
          client_->Write(buffer.get(), buffer->BytesRemaining(),
                         callback.callback(), TRAFFIC_ANNOTATION_FOR_TESTS));
      ASSERT_GT(rv, 0);
      buffer->DidConsume(rv);
    }
  }

  // Wait for the control socket to consume the whole chunk before sending
  // the next one. A single write may arrive in several reads.
  void SendAndWaitForRead(std::string_view chunk) {
    ASSERT_TRUE(control_socket_source_.IsValid());
    net::RecordingNetLogObserver reads;
    ASSERT_NO_FATAL_FAILURE(Send(chunk));
    ASSERT_TRUE(base::test::RunUntil([&] {
      size_t bytes_read = 0;
      for (const auto& entry : reads.GetEntriesForSourceWithType(
               control_socket_source_,
               net::NetLogEventType::SOCKET_BYTES_RECEIVED,
               net::NetLogEventPhase::NONE)) {
        bytes_read += net::GetIntegerValueFromParams(entry, "byte_count");
      }
      return bytes_read == chunk.size();
    }));
    // The socket logs a read just before invoking its callback. Wait for that
    // callback and the delegate notifications it posts to the owner sequence.
    base::test::TestFuture<void> processed;
    io_task_runner_->PostTaskAndReply(FROM_HERE, base::DoNothing(),
                                      processed.GetCallback());
    ASSERT_TRUE(processed.Wait());
  }

  content::BrowserTaskEnvironment task_environment_;
  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;
  testing::NiceMock<MockTorControlDelegate> delegate_;
  std::unique_ptr<TorControl, base::OnTaskRunnerDeleter> control_;
  std::unique_ptr<net::TCPServerSocket> server_;
  std::unique_ptr<net::StreamSocket> client_;
  net::NetLogSource control_socket_source_;
  std::string pending_;
};
}  // namespace

TEST(TorControlTest, ParseQuoted) {
  const struct {
    const char* input;
    const char* output;
    size_t end;
  } cases[] = {
      {"\"127.0.0.1:41159\"", "127.0.0.1:41159", 17},
      {"\"unix:/a b/c\"", "unix:/a b/c", 13},
      {"\"unix:/a\\rb/c\"", "unix:/a\rb/c", 14},
      {"\"unix:/a\\nb/c\"", "unix:/a\nb/c", 14},
      {"\"unix:/a\\tb/c\"", "unix:/a\tb/c", 14},
      {"\"unix:/a\\\\b/c\"", "unix:/a\\b/c", 14},
      {"\"unix:/a\\\"b/c\"", "unix:/a\"b/c", 14},
      {"\"unix:/a\\'b/c\"", "unix:/a'b/c", 14},
      {"\"unix:/a b/c\" \"127.0.0.1:9050\"", "unix:/a b/c", 13},
      {"\"unix:/a b/c", nullptr, static_cast<size_t>(-1)},
      {"\"unix:/a\\fb/c\"", nullptr, static_cast<size_t>(-1)},
  };
  size_t i;

  for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    std::string value;
    size_t end;
    bool ok =
        TorControl::ParseQuoted(UNSAFE_TODO(cases[i]).input, &value, &end);
    if (UNSAFE_TODO(cases[i]).output) {
      EXPECT_TRUE(ok) << i;
      EXPECT_EQ(value, UNSAFE_TODO(cases[i]).output) << i;
      EXPECT_EQ(end, UNSAFE_TODO(cases[i]).end) << i;
    } else {
      EXPECT_FALSE(ok) << i;
    }
  }
}

TEST(TorControlTest, ParseKV) {
  const struct {
    const char* input;
    const char* key;
    const char* value;
    size_t end;
  } cases[] = {
      {"foo=bar", "foo", "bar", 7},
      {"foo=\"bar\"", "foo", "bar", 9},
      {"foo=\"bar baz\"", "foo", "bar baz", 13},
      {"foo=\"bar\\\"baz\"", "foo", "bar\"baz", 14},
      {"foo=\"bar\\\"baz\" quux=\"zot\"", "foo", "bar\"baz", 15},
      {"foo=barbaz quux=zot", "foo", "barbaz", 11},
      {"foo=\"bar", nullptr, nullptr, static_cast<size_t>(-1)},
  };
  size_t i;

  for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    std::string key;
    std::string value;
    size_t end;
    bool ok =
        TorControl::ParseKV(UNSAFE_TODO(cases[i]).input, &key, &value, &end);
    if (UNSAFE_TODO(cases[i]).value) {
      EXPECT_TRUE(ok) << i << ": " << UNSAFE_TODO(cases[i]).input
                      << "\nkey  : " << key << "\nvalue: " << value;
      EXPECT_EQ(key, UNSAFE_TODO(cases[i]).key)
          << i << ": " << UNSAFE_TODO(cases[i]).input << "\nkey  : " << key
          << "\nvalue: " << value;
      EXPECT_EQ(value, UNSAFE_TODO(cases[i]).value)
          << i << ": " << UNSAFE_TODO(cases[i]).input << "\nkey  : " << key
          << "\nvalue: " << value;
      EXPECT_EQ(end, UNSAFE_TODO(cases[i]).end)
          << i << ": " << UNSAFE_TODO(cases[i]).input << "\nkey  : " << key
          << "\nvalue: " << value;
    } else {
      EXPECT_FALSE(ok) << i << ": " << UNSAFE_TODO(cases[i]).input
                       << "\nkey  : " << key << "\nvalue: " << value;
    }
  }
}

// Lines split across several writes, including inside a CRLF-less line and
// right after a CR, are each reported once and in order.
TEST_F(TorControlReadTest, LinesSplitAcrossReads) {
  auto calls = base::MakeRefCounted<base::RefCountedData<int>>(0);
  auto count_call = [calls](const std::string&, const std::string&) {
    ++calls->data;
  };
  {
    testing::InSequence in_sequence;
    EXPECT_CALL(delegate_, OnTorRawMid("250", "SOCKSPORT=9050"))
        .WillOnce(count_call);
    EXPECT_CALL(delegate_, OnTorRawEnd("250", "ORPORT=0")).WillOnce(count_call);
    EXPECT_CALL(delegate_, OnTorRawEnd("250", "DONE")).WillOnce(count_call);
  }

  // Each Send() is read before the next one is written. Nothing is reported
  // while the first line is still incomplete, including when its CR is in one
  // read and its LF in the next.
  ASSERT_NO_FATAL_FAILURE(SendAndWaitForRead("250-SOCKSP"));
  EXPECT_EQ(calls->data, 0);
  ASSERT_NO_FATAL_FAILURE(SendAndWaitForRead("ORT=9050\r"));
  EXPECT_EQ(calls->data, 0);
  ASSERT_NO_FATAL_FAILURE(SendAndWaitForRead("\n250 ORPORT=0\r\n2"));
  EXPECT_EQ(calls->data, 2);
  ASSERT_NO_FATAL_FAILURE(SendAndWaitForRead("50 DONE\r\n"));
  EXPECT_EQ(calls->data, 3);
}

// More input than fits in TorControl's read buffer is delivered line by line
// without losing, duplicating or corrupting any line, including an unfinished
// line that is kept across the buffer being full.
TEST_F(TorControlReadTest, InputLargerThanReadBuffer) {
  constexpr int kLineCount = 600;
  std::string input;
  std::vector<std::string> expected_lines;
  for (int i = 0; i < kLineCount; ++i) {
    std::string line = absl::StrFormat("ITEM%03d=%s", i, std::string(20, 'x'));
    input += "250-" + line + "\r\n";
    expected_lines.push_back(std::move(line));
  }
  ASSERT_GT(input.size(), 4096u * 4);

  std::vector<std::string> received_lines;
  base::RunLoop all_mid_loop;
  EXPECT_CALL(delegate_, OnTorRawMid("250", testing::_))
      .Times(kLineCount)
      .WillRepeatedly([&](const std::string& status, const std::string& line) {
        received_lines.push_back(line);
        if (received_lines.size() == expected_lines.size()) {
          all_mid_loop.Quit();
        }
      });
  base::RunLoop end_loop;
  EXPECT_CALL(delegate_, OnTorRawEnd("250", "FINISHED"))
      .WillOnce(base::test::RunClosure(end_loop.QuitClosure()));

  ASSERT_NO_FATAL_FAILURE(Send(input + "250 FINI"));
  all_mid_loop.Run();
  EXPECT_EQ(received_lines, expected_lines);

  ASSERT_NO_FATAL_FAILURE(Send("SHED\r\n"));
  end_loop.Run();
}

// A line that does not fit in the read buffer closes the control connection.
TEST_F(TorControlReadTest, LineTooLongClosesConnection) {
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_, OnTorControlClosed(true))
      .WillOnce(base::test::RunClosure(run_loop.QuitClosure()));
  EXPECT_CALL(delegate_, OnTorRawMid(testing::_, testing::_)).Times(0);
  EXPECT_CALL(delegate_, OnTorRawEnd(testing::_, testing::_)).Times(0);

  ASSERT_NO_FATAL_FAILURE(Send(std::string(5000, 'a')));
  run_loop.Run();
}

// A line feed that does not follow a carriage return closes the connection.
TEST_F(TorControlReadTest, StrayLineFeedClosesConnection) {
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_, OnTorControlClosed(true))
      .WillOnce(base::test::RunClosure(run_loop.QuitClosure()));
  EXPECT_CALL(delegate_, OnTorRawEnd(testing::_, testing::_)).Times(0);

  ASSERT_NO_FATAL_FAILURE(Send("250 DONE\n"));
  run_loop.Run();
}

// A line too short to hold a status code and separator closes the connection.
TEST_F(TorControlReadTest, ShortLineClosesConnection) {
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_, OnTorControlClosed(true))
      .WillOnce(base::test::RunClosure(run_loop.QuitClosure()));
  EXPECT_CALL(delegate_, OnTorRawEnd(testing::_, testing::_)).Times(0);

  ASSERT_NO_FATAL_FAILURE(Send("500\r\n"));
  run_loop.Run();
}

// A status code followed by an unknown separator closes the connection.
TEST_F(TorControlReadTest, UnknownSeparatorClosesConnection) {
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_, OnTorControlClosed(true))
      .WillOnce(base::test::RunClosure(run_loop.QuitClosure()));
  EXPECT_CALL(delegate_, OnTorRawEnd(testing::_, testing::_)).Times(0);

  ASSERT_NO_FATAL_FAILURE(Send("500/OK\r\n"));
  run_loop.Run();
}

// A known event that TorControl is not subscribed to is not reported.
TEST_F(TorControlReadTest, UnsubscribedEventIsNotReported) {
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_, OnTorEvent(testing::_, testing::_, testing::_))
      .Times(0);
  EXPECT_CALL(delegate_, OnTorEvent(TorControlEvent::NETWORK_LIVENESS, "DOWN",
                                    testing::IsEmpty()))
      .WillOnce(base::test::RunClosure(run_loop.QuitClosure()));

  // Handshake() subscribed to NETWORK_LIVENESS but not to CIRC.
  ASSERT_NO_FATAL_FAILURE(
      Send("650 CIRC 1000 EXTENDED\r\n650 NETWORK_LIVENESS DOWN\r\n"));
  run_loop.Run();
}

// A multi-line event with an unknown name is skipped through its final line,
// and the event after it is reported.
TEST_F(TorControlReadTest, UnknownMultiLineEventIsSkipped) {
  base::RunLoop run_loop;
  EXPECT_CALL(delegate_, OnTorRawAsync("650", "FAKEVENT BEGIN"));
  EXPECT_CALL(delegate_, OnTorRawAsync("650", "CONTINUE=FAKEVENT"));
  EXPECT_CALL(delegate_, OnTorRawAsync("650", "END=FAKEVENT"));
  EXPECT_CALL(delegate_, OnTorRawAsync("650", "NETWORK_LIVENESS DOWN"));
  EXPECT_CALL(delegate_, OnTorEvent(testing::_, testing::_, testing::_))
      .Times(0);
  EXPECT_CALL(delegate_, OnTorEvent(TorControlEvent::NETWORK_LIVENESS, "DOWN",
                                    testing::IsEmpty()))
      .WillOnce(base::test::RunClosure(run_loop.QuitClosure()));

  ASSERT_NO_FATAL_FAILURE(
      Send("650-FAKEVENT BEGIN\r\n"
           "650-CONTINUE=FAKEVENT\r\n"
           "650 END=FAKEVENT\r\n"
           "650 NETWORK_LIVENESS DOWN\r\n"));
  run_loop.Run();
}

// A subscribed multi-line event is reported once, with its continuation and
// final lines as key-value pairs.
TEST_F(TorControlReadTest, MultiLineEvent) {
  ASSERT_NO_FATAL_FAILURE(
      Subscribe(TorControlEvent::CIRC, "SETEVENTS CIRC NETWORK_LIVENESS"));

  base::RunLoop run_loop;
  const std::map<std::string, std::string> extra = {{"ANONYMITY", "high"},
                                                    {"EXTRAMAGIC", "99"}};
  EXPECT_CALL(delegate_,
              OnTorEvent(TorControlEvent::CIRC, "1000 EXTENDED", extra))
      .WillOnce(base::test::RunClosure(run_loop.QuitClosure()));

  ASSERT_NO_FATAL_FAILURE(
      Send("650-CIRC 1000 EXTENDED\r\n"
           "650-EXTRAMAGIC=99\r\n"
           "650 ANONYMITY=high\r\n"));
  run_loop.Run();
}

TEST(TorControlTest, GetCircuitEstablishedDone) {
  content::BrowserTaskEnvironment task_environment;
  scoped_refptr<base::SequencedTaskRunner> io_task_runner =
      content::GetIOThreadTaskRunner({});

  MockTorControlDelegate delegate;
  std::unique_ptr<TorControl> control =
      std::make_unique<TorControl>(delegate.AsWeakPtr(), io_task_runner);

  base::RunLoop run_loop;
  io_task_runner->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(
          [](std::unique_ptr<TorControl> control) {
            std::unique_ptr<std::string> established =
                std::make_unique<std::string>("0");
            bool is_called = false;
            control->GetCircuitEstablishedDone(
                std::move(established),
                base::BindOnce(
                    [](bool* is_called, bool error, bool result) {
                      *is_called = true;
                      EXPECT_FALSE(error);
                      EXPECT_FALSE(result);
                    },
                    &is_called),
                false, "250", "OK");
            EXPECT_TRUE(is_called);

            is_called = false;
            established = std::make_unique<std::string>("1");
            control->GetCircuitEstablishedDone(
                std::move(established),
                base::BindOnce(
                    [](bool* is_called, bool error, bool result) {
                      *is_called = true;
                      EXPECT_FALSE(error);
                      EXPECT_TRUE(result);
                    },
                    &is_called),
                false, "250", "OK");
            EXPECT_TRUE(is_called);

            // --- Error cases ---
            is_called = false;
            established = std::make_unique<std::string>("iambrave");
            control->GetCircuitEstablishedDone(
                std::move(established),
                base::BindOnce(
                    [](bool* is_called, bool error, bool result) {
                      *is_called = true;
                      EXPECT_TRUE(error);
                      EXPECT_FALSE(result);
                    },
                    &is_called),
                false, "250", "OK");
            EXPECT_TRUE(is_called);

            is_called = false;
            established = std::make_unique<std::string>("");
            control->GetCircuitEstablishedDone(
                std::move(established),
                base::BindOnce(
                    [](bool* is_called, bool error, bool result) {
                      *is_called = true;
                      EXPECT_TRUE(error);
                      EXPECT_FALSE(result);
                    },
                    &is_called),
                false, "250", "OK");
            EXPECT_TRUE(is_called);

            is_called = false;
            established = std::make_unique<std::string>("1");
            control->GetCircuitEstablishedDone(
                std::move(established),
                base::BindOnce(
                    [](bool* is_called, bool error, bool result) {
                      *is_called = true;
                      EXPECT_TRUE(error);
                      EXPECT_FALSE(result);
                    },
                    &is_called),
                true, "250", "OK");
            EXPECT_TRUE(is_called);

            is_called = false;
            established = std::make_unique<std::string>("1");
            control->GetCircuitEstablishedDone(
                std::move(established),
                base::BindOnce(
                    [](bool* is_called, bool error, bool result) {
                      *is_called = true;
                      EXPECT_TRUE(error);
                      EXPECT_FALSE(result);
                    },
                    &is_called),
                false, "500", "OK");
            EXPECT_TRUE(is_called);

            is_called = false;
            established = std::make_unique<std::string>("1");
            control->GetCircuitEstablishedDone(
                std::move(established),
                base::BindOnce(
                    [](bool* is_called, bool error, bool result) {
                      *is_called = true;
                      EXPECT_TRUE(error);
                      EXPECT_FALSE(result);
                    },
                    &is_called),
                false, "500", "NOT_OK");
            EXPECT_TRUE(is_called);
          },
          std::move(control)),
      run_loop.QuitClosure());
  run_loop.Run();
}

}  // namespace tor
