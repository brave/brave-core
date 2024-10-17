/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "brave/components/tor/tor_control.h"

#include <memory>

#include "base/functional/callback_helpers.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/browser_task_environment.h"
#include "net/base/io_buffer.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace tor {
namespace {
class MockTorControlDelegate final : public TorControl::Delegate {
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
    bool ok = TorControl::ParseQuoted(cases[i].input, &value, &end);
    if (cases[i].output) {
      EXPECT_TRUE(ok) << i;
      EXPECT_EQ(value, cases[i].output) << i;
      EXPECT_EQ(end, cases[i].end) << i;
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
    bool ok = TorControl::ParseKV(cases[i].input, &key, &value, &end);
    if (cases[i].value) {
      EXPECT_TRUE(ok) << i << ": " << cases[i].input << "\nkey  : " << key
                      << "\nvalue: " << value;
      EXPECT_EQ(key, cases[i].key)
          << i << ": " << cases[i].input << "\nkey  : " << key
          << "\nvalue: " << value;
      EXPECT_EQ(value, cases[i].value)
          << i << ": " << cases[i].input << "\nkey  : " << key
          << "\nvalue: " << value;
      EXPECT_EQ(end, cases[i].end)
          << i << ": " << cases[i].input << "\nkey  : " << key
          << "\nvalue: " << value;
    } else {
      EXPECT_FALSE(ok) << i << ": " << cases[i].input << "\nkey  : " << key
                       << "\nvalue: " << value;
    }
  }
}

TEST(TorControlTest, ReadDone) {
  content::BrowserTaskEnvironment task_environment;
  scoped_refptr<base::SequencedTaskRunner> io_task_runner =
      content::GetIOThreadTaskRunner({});
  MockTorControlDelegate delegate;
  std::unique_ptr<TorControl> control =
      std::make_unique<TorControl>(delegate.AsWeakPtr(), io_task_runner);
  base::RunLoop run_loop;
  io_task_runner->PostTask(
      FROM_HERE, base::BindLambdaForTesting([&run_loop, &control, &delegate] {
        constexpr char test_str[] =
            "250-SOCKSPORT=9050\r\n250 ORPORT=0\r\n250 OK";
        control->reading_ = true;
        // StartRead()
        control->read_start_ = 0;
        control->async_events_[tor::TorControlEvent::NETWORK_LIVENESS] = 1;
        control->readiobuf_ = base::MakeRefCounted<net::GrowableIOBuffer>();
        control->readiobuf_->SetCapacity(sizeof test_str - 1);
        std::memcpy(control->readiobuf_->data(), test_str, sizeof test_str - 1);

        // Not yet seen CRLF
        // Read this time: 250-SOCKSP
        // Read so far: 250-SOCKSP
        control->ReadDone(10);
        EXPECT_EQ(control->readiobuf_->offset(), 10);
        EXPECT_EQ(control->read_start_, 0);

        // First CRLF
        // Read this time: ORT=9050\r\n
        // Read so far: 250-SOCKSPORT=9050\r\n
        // ReadLine("250-SOCKSPORT=9050")
        EXPECT_CALL(delegate, OnTorRawMid("250", "SOCKSPORT=9050")).Times(1);
        control->ReadDone(10);
        EXPECT_EQ(control->readiobuf_->offset(), 20);
        EXPECT_EQ(control->read_start_, 20);

        // Second CRLF
        // Read this time: 250 ORPORT=0\r\n2
        // Read so far: 250 ORPORT=0\r\n2
        // ReadLine("250 ORPORT=0")
        EXPECT_CALL(delegate, OnTorRawEnd("250", "ORPORT=0")).Times(1);
        control->ReadDone(15);
        EXPECT_EQ(control->readiobuf_->offset(), 35);
        EXPECT_EQ(control->read_start_, 34);

        // Buffer will be full
        // Read this time: 50 OK
        // Read so far: 250 OK
        control->ReadDone(5);
        EXPECT_EQ(control->readiobuf_->offset(), 6);
        EXPECT_EQ(control->read_start_, 0);
        EXPECT_EQ(base::as_string_view(
                      control->readiobuf_->everything().subspan(0, 6)),
                  "250 OK");
        run_loop.Quit();
      }));
  run_loop.Run();
  // Wait for owner sequence
  base::RunLoop().RunUntilIdle();
}

TEST(TorControlTest, ReadLine) {
  content::BrowserTaskEnvironment task_environment;
  scoped_refptr<base::SequencedTaskRunner> io_task_runner =
      content::GetIOThreadTaskRunner({});

  MockTorControlDelegate delegate;
  std::unique_ptr<TorControl> control =
      std::make_unique<TorControl>(delegate.AsWeakPtr(), io_task_runner);

  EXPECT_CALL(delegate, OnTorControlClosed(false)).Times(2);
  io_task_runner->PostTask(FROM_HERE,
                           base::BindOnce(
                               [](std::unique_ptr<TorControl> control) {
                                 EXPECT_FALSE(control->ReadLine("500"));
                                 EXPECT_FALSE(control->ReadLine("500/OK"));
                               },
                               std::move(control)));

  control = std::make_unique<TorControl>(delegate.AsWeakPtr(), io_task_runner);
  EXPECT_CALL(delegate, OnTorRawMid("250", "SOCKSPORT=9050")).Times(1);
  EXPECT_CALL(delegate, OnTorRawEnd("250", "OK")).Times(1);
  io_task_runner->PostTask(
      FROM_HERE, base::BindOnce(
                     [](std::unique_ptr<TorControl> control) {
                       EXPECT_TRUE(control->ReadLine("250-SOCKSPORT=9050"));
                       EXPECT_TRUE(control->ReadLine("250 OK"));
                     },
                     std::move(control)));

  // Test Async:
  control = std::make_unique<TorControl>(delegate.AsWeakPtr(), io_task_runner);
  using tor::TorControlEvent;
  EXPECT_CALL(delegate, OnTorRawAsync("650", "FAKEVENT WHAT")).Times(1);
  EXPECT_CALL(delegate, OnTorRawAsync("650", "NETWORK_LIVENESS UP")).Times(1);
  EXPECT_CALL(delegate, OnTorRawAsync("650", "NETWORK_LIVENESS DOWN")).Times(1);
  EXPECT_CALL(delegate, OnTorRawAsync("650", "FAKEVENT BEGIN")).Times(1);
  EXPECT_CALL(delegate, OnTorRawAsync("650", "CONTINUE=FAKEVENT")).Times(1);
  EXPECT_CALL(delegate, OnTorRawAsync("650", "END=FAKEVENT")).Times(1);
  EXPECT_CALL(delegate, OnTorRawAsync("650", "CIRC 1000 EXTENDED")).Times(1);
  EXPECT_CALL(delegate, OnTorRawAsync("650", "EXTRAMAGIC=99")).Times(1);
  EXPECT_CALL(delegate, OnTorRawAsync("650", "ANONYMITY=high")).Times(1);
  EXPECT_CALL(delegate,
              OnTorEvent(TorControlEvent::NETWORK_LIVENESS, "DOWN", testing::_))
      .Times(1);
  std::map<std::string, std::string> circ_extra = {{"ANONYMITY", "high"},
                                                   {"EXTRAMAGIC", "99"}};
  EXPECT_CALL(delegate,
              OnTorEvent(TorControlEvent::CIRC, "1000 EXTENDED", circ_extra))
      .Times(1);
  io_task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](std::unique_ptr<TorControl> control) {
            EXPECT_FALSE(control->ReadLine("650 FAKEVENT WHAT"));
            EXPECT_TRUE(control->ReadLine("650 NETWORK_LIVENESS UP"));
            // Emulate subscribe
            control->async_events_[TorControlEvent::NETWORK_LIVENESS] = 1;
            EXPECT_TRUE(control->ReadLine("650 NETWORK_LIVENESS DOWN"));
            // Async skip
            EXPECT_TRUE(control->ReadLine("650-FAKEVENT BEGIN"));
            EXPECT_TRUE(control->async_);
            EXPECT_TRUE(control->async_->skip);
            EXPECT_TRUE(control->ReadLine("650-CONTINUE=FAKEVENT"));
            EXPECT_TRUE(control->ReadLine("650 END=FAKEVENT"));
            EXPECT_FALSE(control->async_);
            // Normal multi async
            control->async_events_[TorControlEvent::CIRC] = 1;
            EXPECT_TRUE(control->ReadLine("650-CIRC 1000 EXTENDED"));
            EXPECT_TRUE(control->async_);
            EXPECT_FALSE(control->async_->skip);
            EXPECT_TRUE(control->ReadLine("650-EXTRAMAGIC=99"));
            EXPECT_TRUE(control->ReadLine("650 ANONYMITY=high"));
            EXPECT_FALSE(control->async_);
          },
          std::move(control)));

  base::RunLoop().RunUntilIdle();
}

TEST(TorControlTest, GetCircuitEstablishedDone) {
  content::BrowserTaskEnvironment task_environment;
  scoped_refptr<base::SequencedTaskRunner> io_task_runner =
      content::GetIOThreadTaskRunner({});

  MockTorControlDelegate delegate;
  std::unique_ptr<TorControl> control =
      std::make_unique<TorControl>(delegate.AsWeakPtr(), io_task_runner);

  io_task_runner->PostTask(
      FROM_HERE, base::BindOnce(
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
                     std::move(control)));
  base::RunLoop().RunUntilIdle();
}

}  // namespace tor
