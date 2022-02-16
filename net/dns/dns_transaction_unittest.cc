/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "net/dns/dns_transaction.h"

#include <stdint.h>

#include <limits>
#include <utility>
#include <vector>

#include "base/base64url.h"
#include "base/bind.h"
#include "base/containers/circular_deque.h"
#include "base/cxx17_backports.h"
#include "base/memory/raw_ptr.h"
#include "base/rand_util.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/sys_byteorder.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/net/decentralized_dns/constants.h"
#include "net/base/ip_address.h"
#include "net/base/port_util.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/base/url_util.h"
#include "net/cookies/cookie_access_result.h"
#include "net/cookies/cookie_util.h"
#include "net/dns/dns_config.h"
#include "net/dns/dns_query.h"
#include "net/dns/dns_response.h"
#include "net/dns/dns_server_iterator.h"
#include "net/dns/dns_session.h"
#include "net/dns/dns_socket_allocator.h"
#include "net/dns/dns_test_util.h"
#include "net/dns/dns_util.h"
#include "net/dns/public/dns_over_https_server_config.h"
#include "net/dns/public/dns_protocol.h"
#include "net/dns/resolve_context.h"
#include "net/http/http_util.h"
#include "net/log/net_log.h"
#include "net/log/net_log_capture_mode.h"
#include "net/log/net_log_with_source.h"
#include "net/proxy_resolution/proxy_config_service_fixed.h"
#include "net/socket/socket_test_util.h"
#include "net/test/gtest_util.h"
#include "net/test/test_with_task_environment.h"
#include "net/test/url_request/url_request_failed_job.h"
#include "net/third_party/uri_template/uri_template.h"
#include "net/url_request/url_request_filter.h"
#include "net/url_request/url_request_interceptor.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

using net::test::IsOk;

namespace net {

namespace {

base::TimeDelta kFallbackPeriod = base::Seconds(1);

const char kMockHostname[] = "mock.http";

std::string DomainFromDot(const base::StringPiece& dotted) {
  std::string out;
  EXPECT_TRUE(DNSDomainFromDot(dotted, &out));
  return out;
}

enum class Transport { UDP, TCP, HTTPS };

// A SocketDataProvider builder.
class DnsSocketData {
 public:
  // The ctor takes parameters for the DnsQuery.
  DnsSocketData(uint16_t id,
                const char* dotted_name,
                uint16_t qtype,
                IoMode mode,
                Transport transport,
                const OptRecordRdata* opt_rdata = nullptr,
                DnsQuery::PaddingStrategy padding_strategy =
                    DnsQuery::PaddingStrategy::NONE)
      : query_(new DnsQuery(id,
                            DomainFromDot(dotted_name),
                            qtype,
                            opt_rdata,
                            padding_strategy)),
        transport_(transport) {
    if (Transport::TCP == transport_) {
      std::unique_ptr<uint16_t> length(new uint16_t);
      *length = base::HostToNet16(query_->io_buffer()->size());
      writes_.push_back(MockWrite(mode,
                                  reinterpret_cast<const char*>(length.get()),
                                  sizeof(uint16_t), num_reads_and_writes()));
      lengths_.push_back(std::move(length));
    }
    writes_.push_back(MockWrite(mode, query_->io_buffer()->data(),
                                query_->io_buffer()->size(),
                                num_reads_and_writes()));
  }

  DnsSocketData(const DnsSocketData&) = delete;

  DnsSocketData& operator=(const DnsSocketData&) = delete;

  ~DnsSocketData() = default;

  // All responses must be added before GetProvider.

  // Adds pre-built DnsResponse. |tcp_length| will be used in TCP mode only.
  void AddResponseWithLength(std::unique_ptr<DnsResponse> response,
                             IoMode mode,
                             uint16_t tcp_length) {
    CHECK(!provider_.get());
    if (Transport::TCP == transport_) {
      std::unique_ptr<uint16_t> length(new uint16_t);
      *length = base::HostToNet16(tcp_length);
      reads_.push_back(MockRead(mode,
                                reinterpret_cast<const char*>(length.get()),
                                sizeof(uint16_t), num_reads_and_writes()));
      lengths_.push_back(std::move(length));
    }
    reads_.push_back(MockRead(mode, response->io_buffer()->data(),
                              response->io_buffer_size(),
                              num_reads_and_writes()));
    responses_.push_back(std::move(response));
  }

  // Adds pre-built DnsResponse.
  void AddResponse(std::unique_ptr<DnsResponse> response, IoMode mode) {
    uint16_t tcp_length = response->io_buffer_size();
    AddResponseWithLength(std::move(response), mode, tcp_length);
  }

  // Adds pre-built response from |data| buffer.
  void AddResponseData(const uint8_t* data, size_t length, IoMode mode) {
    CHECK(!provider_.get());
    AddResponse(std::make_unique<DnsResponse>(
                    reinterpret_cast<const char*>(data), length, 0),
                mode);
  }

  // Adds pre-built response from |data| buffer.
  void AddResponseData(const uint8_t* data,
                       size_t length,
                       int offset,
                       IoMode mode) {
    CHECK(!provider_.get());
    AddResponse(
        std::make_unique<DnsResponse>(reinterpret_cast<const char*>(data),
                                      length - offset, offset),
        mode);
  }

  // Add no-answer (RCODE only) response matching the query.
  void AddRcode(int rcode, IoMode mode) {
    std::unique_ptr<DnsResponse> response(new DnsResponse(
        query_->io_buffer()->data(), query_->io_buffer()->size(), 0));
    dns_protocol::Header* header =
        reinterpret_cast<dns_protocol::Header*>(response->io_buffer()->data());
    header->flags |= base::HostToNet16(dns_protocol::kFlagResponse | rcode);
    AddResponse(std::move(response), mode);
  }

  // Add error response.
  void AddReadError(int error, IoMode mode) {
    reads_.push_back(MockRead(mode, error, num_reads_and_writes()));
  }

  // Build, if needed, and return the SocketDataProvider. No new responses
  // should be added afterwards.
  SequencedSocketData* GetProvider() {
    if (provider_.get())
      return provider_.get();
    // Terminate the reads with ERR_IO_PENDING to prevent overrun and default to
    // timeout.
    if (transport_ != Transport::HTTPS) {
      reads_.push_back(MockRead(SYNCHRONOUS, ERR_IO_PENDING,
                                writes_.size() + reads_.size()));
    }
    provider_.reset(new SequencedSocketData(reads_, writes_));
    if (Transport::TCP == transport_ || Transport::HTTPS == transport_) {
      provider_->set_connect_data(MockConnect(reads_[0].mode, OK));
    }
    return provider_.get();
  }

  uint16_t query_id() const { return query_->id(); }

  IOBufferWithSize* query_buffer() { return query_->io_buffer(); }

 private:
  size_t num_reads_and_writes() const { return reads_.size() + writes_.size(); }

  std::unique_ptr<DnsQuery> query_;
  Transport transport_;
  std::vector<std::unique_ptr<uint16_t>> lengths_;
  std::vector<std::unique_ptr<DnsResponse>> responses_;
  std::vector<MockWrite> writes_;
  std::vector<MockRead> reads_;
  std::unique_ptr<SequencedSocketData> provider_;
};

class TestSocketFactory;

// A variant of MockUDPClientSocket which always fails to Connect.
class FailingUDPClientSocket : public MockUDPClientSocket {
 public:
  FailingUDPClientSocket(SocketDataProvider* data, net::NetLog* net_log)
      : MockUDPClientSocket(data, net_log) {}
  FailingUDPClientSocket(const FailingUDPClientSocket&) = delete;
  FailingUDPClientSocket& operator=(const FailingUDPClientSocket&) = delete;
  ~FailingUDPClientSocket() override = default;
  int Connect(const IPEndPoint& endpoint) override {
    return ERR_CONNECTION_REFUSED;
  }
};

// A variant of MockUDPClientSocket which notifies the factory OnConnect.
class TestUDPClientSocket : public MockUDPClientSocket {
 public:
  TestUDPClientSocket(TestSocketFactory* factory,
                      SocketDataProvider* data,
                      net::NetLog* net_log)
      : MockUDPClientSocket(data, net_log), factory_(factory) {}
  TestUDPClientSocket(const TestUDPClientSocket&) = delete;
  TestUDPClientSocket& operator=(const TestUDPClientSocket&) = delete;
  ~TestUDPClientSocket() override = default;
  int Connect(const IPEndPoint& endpoint) override;

 private:
  raw_ptr<TestSocketFactory> factory_ = nullptr;
};

// Creates TestUDPClientSockets and keeps endpoints reported via OnConnect.
class TestSocketFactory : public MockClientSocketFactory {
 public:
  TestSocketFactory() = default;
  ~TestSocketFactory() override = default;

  std::unique_ptr<DatagramClientSocket> CreateDatagramClientSocket(
      DatagramSocket::BindType bind_type,
      NetLog* net_log,
      const NetLogSource& source) override {
    if (fail_next_socket_) {
      fail_next_socket_ = false;
      return std::unique_ptr<DatagramClientSocket>(
          new FailingUDPClientSocket(&empty_data_, net_log));
    }

    SocketDataProvider* data_provider = mock_data().GetNext();
    auto socket =
        std::make_unique<TestUDPClientSocket>(this, data_provider, net_log);

    // Even using DEFAULT_BIND, actual sockets have been measured to very rarely
    // repeat the same source port multiple times in a row. Need to mimic that
    // functionality here, so DnsUdpTracker doesn't misdiagnose repeated port
    // as low entropy.
    if (diverse_source_ports_)
      socket->set_source_port(next_source_port_++);

    return socket;
  }

  void OnConnect(const IPEndPoint& endpoint) {
    remote_endpoints_.emplace_back(endpoint);
  }

  struct RemoteNameserver {
    explicit RemoteNameserver(IPEndPoint insecure_nameserver)
        : insecure_nameserver(insecure_nameserver) {}
    explicit RemoteNameserver(DnsOverHttpsServerConfig secure_nameserver)
        : secure_nameserver(secure_nameserver) {}

    absl::optional<IPEndPoint> insecure_nameserver;
    absl::optional<DnsOverHttpsServerConfig> secure_nameserver;
  };

  std::vector<RemoteNameserver> remote_endpoints_;
  bool fail_next_socket_ = false;
  bool diverse_source_ports_ = true;

 private:
  StaticSocketDataProvider empty_data_;
  uint16_t next_source_port_ = 123;
};

int TestUDPClientSocket::Connect(const IPEndPoint& endpoint) {
  factory_->OnConnect(endpoint);
  return MockUDPClientSocket::Connect(endpoint);
}

// Helper class that holds a DnsTransaction and handles OnTransactionComplete.
class TransactionHelper {
 public:
  // If |expected_answer_count| < 0 then it is the expected net error.
  explicit TransactionHelper(int expected_answer_count)
      : expected_answer_count_(expected_answer_count) {}

  // Mark that the transaction shall be destroyed immediately upon callback.
  void set_cancel_in_callback() { cancel_in_callback_ = true; }

  void StartTransaction(DnsTransactionFactory* factory,
                        const char* hostname,
                        uint16_t qtype,
                        bool secure,
                        ResolveContext* context) {
    std::unique_ptr<DnsTransaction> transaction = factory->CreateTransaction(
        hostname, qtype, CompletionCallback(),
        NetLogWithSource::Make(net::NetLog::Get(), net::NetLogSourceType::NONE),
        secure, factory->GetSecureDnsModeForTest(), context,
        true /* fast_timeout */);
    transaction->SetRequestPriority(DEFAULT_PRIORITY);
    EXPECT_EQ(qtype, transaction->GetType());
    StartTransaction(std::move(transaction));
  }

  void StartTransaction(std::unique_ptr<DnsTransaction> transaction) {
    EXPECT_FALSE(transaction_);
    transaction_ = std::move(transaction);
    qtype_ = transaction_->GetType();
    transaction_->Start();
  }

  void Cancel() {
    ASSERT_TRUE(transaction_.get() != nullptr);
    transaction_.reset(nullptr);
  }

  DnsTransactionFactory::CallbackType CompletionCallback() {
    return base::BindOnce(&TransactionHelper::OnTransactionComplete,
                          base::Unretained(this));
  }

  void OnTransactionComplete(DnsTransaction* t,
                             int rv,
                             const DnsResponse* response,
                             absl::optional<std::string> doh_provider_id) {
    EXPECT_FALSE(completed_);
    EXPECT_EQ(transaction_.get(), t);

    completed_ = true;
    response_ = response;

    transaction_complete_run_loop_.Quit();

    if (cancel_in_callback_) {
      Cancel();
      return;
    }

    if (response)
      EXPECT_TRUE(response->IsValid());

    if (expected_answer_count_ >= 0) {
      ASSERT_THAT(rv, IsOk());
      ASSERT_TRUE(response != nullptr);
      EXPECT_EQ(static_cast<unsigned>(expected_answer_count_),
                response->answer_count());
      EXPECT_EQ(qtype_, response->GetSingleQType());

      DnsRecordParser parser = response->Parser();
      DnsResourceRecord record;
      for (int i = 0; i < expected_answer_count_; ++i) {
        EXPECT_TRUE(parser.ReadRecord(&record));
      }
    } else {
      EXPECT_EQ(expected_answer_count_, rv);
    }
  }

  bool has_completed() const { return completed_; }
  const DnsResponse* response() const { return response_; }

  // Runs until the completion callback is called. Transaction must have already
  // been started or this will never complete.
  void RunUntilComplete() {
    DCHECK(transaction_);
    DCHECK(!transaction_complete_run_loop_.running());
    transaction_complete_run_loop_.Run();
    DCHECK(has_completed());
  }

 private:
  uint16_t qtype_ = 0;
  std::unique_ptr<DnsTransaction> transaction_;
  const DnsResponse* response_ = nullptr;
  int expected_answer_count_;
  bool cancel_in_callback_ = false;
  base::RunLoop transaction_complete_run_loop_;
  bool completed_ = false;
};

// Callback that allows a test to modify HttpResponseinfo
// before the response is sent to the requester. This allows
// response headers to be changed.
typedef base::RepeatingCallback<void(URLRequest* request,
                                     HttpResponseInfo* info)>
    ResponseModifierCallback;

// Callback that allows the test to substitute its own implementation
// of URLRequestJob to handle the request.
typedef base::RepeatingCallback<std::unique_ptr<URLRequestJob>(
    URLRequest* request,
    SocketDataProvider* data_provider)>
    DohJobMakerCallback;

// Subclass of URLRequestJob which takes a SocketDataProvider with data
// representing both a DNS over HTTPS query and response.
class URLRequestMockDohJob : public URLRequestJob, public AsyncSocket {
 public:
  URLRequestMockDohJob(
      URLRequest* request,
      SocketDataProvider* data_provider,
      ResponseModifierCallback response_modifier = ResponseModifierCallback())
      : URLRequestJob(request),
        content_length_(0),
        leftover_data_len_(0),
        data_provider_(data_provider),
        response_modifier_(response_modifier) {
    data_provider_->Initialize(this);
    MatchQueryData(request, data_provider);
  }

  URLRequestMockDohJob(const URLRequestMockDohJob&) = delete;

  URLRequestMockDohJob& operator=(const URLRequestMockDohJob&) = delete;

  // Compare the query contained in either the POST body or the body
  // parameter of the GET query to the write data of the SocketDataProvider.
  static void MatchQueryData(URLRequest* request,
                             SocketDataProvider* data_provider) {
    std::string decoded_query;
    if (request->method() == "GET") {
      std::string encoded_query;
      EXPECT_TRUE(GetValueForKeyInQuery(request->url(), "dns", &encoded_query));
      EXPECT_GT(encoded_query.size(), 0ul);

      EXPECT_TRUE(base::Base64UrlDecode(
          encoded_query, base::Base64UrlDecodePolicy::IGNORE_PADDING,
          &decoded_query));
    } else if (request->method() == "POST") {
      const UploadDataStream* stream = request->get_upload_for_testing();
      auto* readers = stream->GetElementReaders();
      EXPECT_TRUE(readers);
      EXPECT_FALSE(readers->empty());
      for (auto& reader : *readers) {
        const UploadBytesElementReader* byte_reader = reader->AsBytesReader();
        decoded_query +=
            std::string(byte_reader->bytes(), byte_reader->length());
      }
    }

    std::string query(decoded_query);
    MockWriteResult result(SYNCHRONOUS, 1);
    while (result.result > 0 && query.length() > 0) {
      result = data_provider->OnWrite(query);
      if (result.result > 0)
        query = query.substr(result.result);
    }
  }

  static std::string GetMockHttpsUrl(const std::string& path) {
    return "https://" + (kMockHostname + ("/" + path));
  }

  // URLRequestJob implementation:
  void Start() override {
    // Start reading asynchronously so that all error reporting and data
    // callbacks happen as they would for network requests.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&URLRequestMockDohJob::StartAsync,
                                  weak_factory_.GetWeakPtr()));
  }

  ~URLRequestMockDohJob() override {
    if (data_provider_)
      data_provider_->DetachSocket();
  }

  int ReadRawData(IOBuffer* buf, int buf_size) override {
    if (!data_provider_)
      return ERR_FAILED;
    if (leftover_data_len_ > 0) {
      int rv = DoBufferCopy(leftover_data_, leftover_data_len_, buf, buf_size);
      return rv;
    }

    if (data_provider_->AllReadDataConsumed())
      return 0;

    MockRead read = data_provider_->OnRead();

    if (read.result < ERR_IO_PENDING)
      return read.result;

    if (read.result == ERR_IO_PENDING) {
      pending_buf_ = buf;
      pending_buf_size_ = buf_size;
      return ERR_IO_PENDING;
    }
    return DoBufferCopy(read.data, read.data_len, buf, buf_size);
  }

  void GetResponseInfo(HttpResponseInfo* info) override {
    // Send back mock headers.
    std::string raw_headers;
    raw_headers.append(
        "HTTP/1.1 200 OK\n"
        "Content-type: application/dns-message\n");
    if (content_length_ > 0) {
      raw_headers.append(base::StringPrintf("Content-Length: %1d\n",
                                            static_cast<int>(content_length_)));
    }
    info->headers = base::MakeRefCounted<HttpResponseHeaders>(
        HttpUtil::AssembleRawHeaders(raw_headers));
    if (response_modifier_)
      response_modifier_.Run(request(), info);
  }

  // AsyncSocket implementation:
  void OnReadComplete(const MockRead& data) override {
    EXPECT_NE(data.result, ERR_IO_PENDING);
    if (data.result < 0)
      return ReadRawDataComplete(data.result);
    ReadRawDataComplete(DoBufferCopy(data.data, data.data_len, pending_buf_,
                                     pending_buf_size_));
  }
  void OnWriteComplete(int rv) override {}
  void OnConnectComplete(const MockConnect& data) override {}
  void OnDataProviderDestroyed() override { data_provider_ = nullptr; }

 private:
  void StartAsync() {
    if (!request_)
      return;
    if (content_length_)
      set_expected_content_size(content_length_);
    NotifyHeadersComplete();
  }

  int DoBufferCopy(const char* data,
                   int data_len,
                   IOBuffer* buf,
                   int buf_size) {
    if (data_len > buf_size) {
      memcpy(buf->data(), data, buf_size);
      leftover_data_ = data + buf_size;
      leftover_data_len_ = data_len - buf_size;
      return buf_size;
    }
    memcpy(buf->data(), data, data_len);
    return data_len;
  }

  const int content_length_;
  const char* leftover_data_;
  int leftover_data_len_;
  raw_ptr<SocketDataProvider> data_provider_ = nullptr;
  const ResponseModifierCallback response_modifier_;
  raw_ptr<IOBuffer> pending_buf_ = nullptr;
  int pending_buf_size_;

  base::WeakPtrFactory<URLRequestMockDohJob> weak_factory_{this};
};

class DnsTransactionTestBase : public testing::Test {
 public:
  DnsTransactionTestBase() = default;

  ~DnsTransactionTestBase() override {
    // All queued transaction IDs should be used by a transaction calling
    // GetNextId().
    CHECK(transaction_ids_.empty());
  }

  // Generates |nameservers| for DnsConfig.
  void ConfigureNumServers(size_t num_servers) {
    CHECK_LE(num_servers, 255u);
    config_.nameservers.clear();
    for (size_t i = 0; i < num_servers; ++i) {
      config_.nameservers.push_back(
          IPEndPoint(IPAddress(192, 168, 1, i), dns_protocol::kDefaultPort));
    }
  }

  // Configures the DnsConfig DNS-over-HTTPS server(s), which either
  // accept GET or POST requests based on use_post. If a
  // ResponseModifierCallback is provided it will be called to construct the
  // HTTPResponse.
  void ConfigureDohServers(bool use_post,
                           size_t num_doh_servers = 1,
                           bool make_available = true) {
    GURL url(URLRequestMockDohJob::GetMockHttpsUrl("doh_test"));
    URLRequestFilter* filter = URLRequestFilter::GetInstance();
    filter->AddHostnameInterceptor(url.scheme(), url.host(),
                                   std::make_unique<DohJobInterceptor>(this));
    CHECK_LE(num_doh_servers, 255u);
    for (size_t i = 0; i < num_doh_servers; ++i) {
      std::string server_template(URLRequestMockDohJob::GetMockHttpsUrl(
                                      base::StringPrintf("doh_test_%zu", i)) +
                                  "{?dns}");
      config_.dns_over_https_servers.push_back(
          *DnsOverHttpsServerConfig::FromString(server_template));
    }
    ConfigureFactory();

    if (make_available) {
      for (size_t server_index = 0; server_index < num_doh_servers;
           ++server_index) {
        resolve_context_->RecordServerSuccess(
            server_index, true /* is_doh_server */, session_.get());
      }
    }
  }

  // Called after fully configuring |config|.
  void ConfigureFactory() {
    socket_factory_.reset(new TestSocketFactory());
    session_ = new DnsSession(
        config_,
        std::make_unique<DnsSocketAllocator>(
            socket_factory_.get(), config_.nameservers, nullptr /* net_log */),
        base::BindRepeating(&DnsTransactionTestBase::GetNextId,
                            base::Unretained(this)),
        nullptr /* NetLog */);
    resolve_context_->InvalidateCachesAndPerSessionData(
        session_.get(), false /* network_change */);
    transaction_factory_ = DnsTransactionFactory::CreateFactory(session_.get());
  }

  void AddSocketData(std::unique_ptr<DnsSocketData> data,
                     bool enqueue_transaction_id = true) {
    CHECK(socket_factory_.get());
    if (enqueue_transaction_id)
      transaction_ids_.push_back(data->query_id());
    socket_factory_->AddSocketDataProvider(data->GetProvider());
    socket_data_.push_back(std::move(data));
  }

  // Add expected query for |dotted_name| and |qtype| with |id| and response
  // taken verbatim from |data| of |data_length| bytes. The transaction id in
  // |data| should equal |id|, unless testing mismatched response.
  void AddQueryAndResponse(uint16_t id,
                           const char* dotted_name,
                           uint16_t qtype,
                           const uint8_t* response_data,
                           size_t response_length,
                           IoMode mode,
                           Transport transport,
                           const OptRecordRdata* opt_rdata = nullptr,
                           DnsQuery::PaddingStrategy padding_strategy =
                               DnsQuery::PaddingStrategy::NONE,
                           bool enqueue_transaction_id = true) {
    CHECK(socket_factory_.get());
    std::unique_ptr<DnsSocketData> data(new DnsSocketData(
        id, dotted_name, qtype, mode, transport, opt_rdata, padding_strategy));
    data->AddResponseData(response_data, response_length, mode);
    AddSocketData(std::move(data), enqueue_transaction_id);
  }

  void AddQueryAndErrorResponse(uint16_t id,
                                const char* dotted_name,
                                uint16_t qtype,
                                int error,
                                IoMode mode,
                                Transport transport,
                                const OptRecordRdata* opt_rdata = nullptr,
                                DnsQuery::PaddingStrategy padding_strategy =
                                    DnsQuery::PaddingStrategy::NONE,
                                bool enqueue_transaction_id = true) {
    CHECK(socket_factory_.get());
    std::unique_ptr<DnsSocketData> data(new DnsSocketData(
        id, dotted_name, qtype, mode, transport, opt_rdata, padding_strategy));
    data->AddReadError(error, mode);
    AddSocketData(std::move(data), enqueue_transaction_id);
  }

  void AddAsyncQueryAndResponse(uint16_t id,
                                const char* dotted_name,
                                uint16_t qtype,
                                const uint8_t* data,
                                size_t data_length,
                                const OptRecordRdata* opt_rdata = nullptr) {
    AddQueryAndResponse(id, dotted_name, qtype, data, data_length, ASYNC,
                        Transport::UDP, opt_rdata);
  }

  void AddSyncQueryAndResponse(uint16_t id,
                               const char* dotted_name,
                               uint16_t qtype,
                               const uint8_t* data,
                               size_t data_length,
                               const OptRecordRdata* opt_rdata = nullptr) {
    AddQueryAndResponse(id, dotted_name, qtype, data, data_length, SYNCHRONOUS,
                        Transport::UDP, opt_rdata);
  }

  // Add expected query of |dotted_name| and |qtype| and no response.
  void AddHangingQuery(
      const char* dotted_name,
      uint16_t qtype,
      DnsQuery::PaddingStrategy padding_strategy =
          DnsQuery::PaddingStrategy::NONE,
      uint16_t id = base::RandInt(0, std::numeric_limits<uint16_t>::max()),
      bool enqueue_transaction_id = true) {
    std::unique_ptr<DnsSocketData> data(
        new DnsSocketData(id, dotted_name, qtype, ASYNC, Transport::UDP,
                          nullptr /* opt_rdata */, padding_strategy));
    AddSocketData(std::move(data), enqueue_transaction_id);
  }

  // Add expected query of |dotted_name| and |qtype| and matching response with
  // no answer and RCODE set to |rcode|. The id will be generated randomly.
  void AddQueryAndRcode(
      const char* dotted_name,
      uint16_t qtype,
      int rcode,
      IoMode mode,
      Transport trans,
      DnsQuery::PaddingStrategy padding_strategy =
          DnsQuery::PaddingStrategy::NONE,
      uint16_t id = base::RandInt(0, std::numeric_limits<uint16_t>::max()),
      bool enqueue_transaction_id = true) {
    CHECK_NE(dns_protocol::kRcodeNOERROR, rcode);
    std::unique_ptr<DnsSocketData> data(
        new DnsSocketData(id, dotted_name, qtype, mode, trans,
                          nullptr /* opt_rdata */, padding_strategy));
    data->AddRcode(rcode, mode);
    AddSocketData(std::move(data), enqueue_transaction_id);
  }

  void AddAsyncQueryAndRcode(const char* dotted_name,
                             uint16_t qtype,
                             int rcode) {
    AddQueryAndRcode(dotted_name, qtype, rcode, ASYNC, Transport::UDP);
  }

  void AddSyncQueryAndRcode(const char* dotted_name,
                            uint16_t qtype,
                            int rcode) {
    AddQueryAndRcode(dotted_name, qtype, rcode, SYNCHRONOUS, Transport::UDP);
  }

  // Checks if the sockets were connected in the order matching the indices in
  // |servers|.
  void CheckServerOrder(const size_t* servers, size_t num_attempts) {
    ASSERT_EQ(num_attempts, socket_factory_->remote_endpoints_.size());
    auto num_insecure_nameservers = session_->config().nameservers.size();
    for (size_t i = 0; i < num_attempts; ++i) {
      if (servers[i] < num_insecure_nameservers) {
        // Check insecure server match.
        EXPECT_EQ(
            socket_factory_->remote_endpoints_[i].insecure_nameserver.value(),
            session_->config().nameservers[servers[i]]);
      } else {
        // Check secure server match.
        EXPECT_EQ(
            socket_factory_->remote_endpoints_[i].secure_nameserver.value(),
            session_->config()
                .dns_over_https_servers[servers[i] - num_insecure_nameservers]);
      }
    }
  }

  std::unique_ptr<URLRequestJob> MaybeInterceptRequest(URLRequest* request) {
    // If the path indicates a redirect, skip checking the list of
    // configured servers, because it won't be there and we still want
    // to handle it.
    bool server_found = request->url().path() == "/redirect-destination";
    for (auto server : config_.dns_over_https_servers) {
      if (server_found)
        break;
      std::string url_base =
          GetURLFromTemplateWithoutParameters(server.server_template());
      if (server.use_post() && request->method() == "POST") {
        if (url_base == request->url().spec()) {
          server_found = true;
          socket_factory_->remote_endpoints_.emplace_back(server);
        }
      } else if (!server.use_post() && request->method() == "GET") {
        std::string prefix = url_base + "?dns=";
        auto mispair = std::mismatch(prefix.begin(), prefix.end(),
                                     request->url().spec().begin());
        if (mispair.first == prefix.end()) {
          server_found = true;
          socket_factory_->remote_endpoints_.emplace_back(server);
        }
      }
    }
    EXPECT_TRUE(server_found);

    EXPECT_TRUE(
        request->isolation_info().network_isolation_key().IsTransient());

    // All DoH requests for the same ResolveContext should use the same
    // IsolationInfo, so network objects like sockets can be reused between
    // requests.
    if (!expect_multiple_isolation_infos_) {
      if (!isolation_info_) {
        isolation_info_ =
            std::make_unique<IsolationInfo>(request->isolation_info());
      } else {
        EXPECT_TRUE(
            isolation_info_->IsEqualForTesting(request->isolation_info()));
      }
    }

    EXPECT_FALSE(request->allow_credentials());
    EXPECT_EQ(SecureDnsPolicy::kBootstrap, request->secure_dns_policy());

    std::string accept;
    EXPECT_TRUE(request->extra_request_headers().GetHeader("Accept", &accept));
    EXPECT_EQ(accept, "application/dns-message");

    std::string language;
    EXPECT_TRUE(request->extra_request_headers().GetHeader("Accept-Language",
                                                           &language));
    EXPECT_EQ(language, "*");

    std::string user_agent;
    EXPECT_TRUE(
        request->extra_request_headers().GetHeader("User-Agent", &user_agent));
    EXPECT_EQ(user_agent, "Chrome");

    SocketDataProvider* provider = socket_factory_->mock_data().GetNext();

    if (doh_job_maker_)
      return doh_job_maker_.Run(request, provider);

    return std::make_unique<URLRequestMockDohJob>(request, provider,
                                                  response_modifier_);
  }

  class DohJobInterceptor : public URLRequestInterceptor {
   public:
    explicit DohJobInterceptor(DnsTransactionTestBase* test) : test_(test) {}
    DohJobInterceptor(const DohJobInterceptor&) = delete;
    DohJobInterceptor& operator=(const DohJobInterceptor&) = delete;
    ~DohJobInterceptor() override {}

    // URLRequestInterceptor implementation:
    std::unique_ptr<URLRequestJob> MaybeInterceptRequest(
        URLRequest* request) const override {
      return test_->MaybeInterceptRequest(request);
    }

   private:
    raw_ptr<DnsTransactionTestBase> test_ = nullptr;
  };

  void SetResponseModifierCallback(ResponseModifierCallback response_modifier) {
    response_modifier_ = response_modifier;
  }

  void SetDohJobMakerCallback(DohJobMakerCallback doh_job_maker) {
    doh_job_maker_ = doh_job_maker;
  }

  void SetUp() override {
    // By default set one server,
    ConfigureNumServers(1);
    // and no retransmissions,
    config_.attempts = 1;
    // and an arbitrary fallback period.
    config_.fallback_period = kFallbackPeriod;

    request_context_ = std::make_unique<TestURLRequestContext>();
    resolve_context_ = std::make_unique<ResolveContext>(
        request_context_.get(), false /* enable_caching */);

    ConfigureFactory();
  }

  void TearDown() override {
    // Check that all socket data was at least written to.
    for (size_t i = 0; i < socket_data_.size(); ++i) {
      EXPECT_TRUE(socket_data_[i]->GetProvider()->AllWriteDataConsumed()) << i;
    }

    URLRequestFilter* filter = URLRequestFilter::GetInstance();
    filter->ClearHandlers();
  }

  void set_expect_multiple_isolation_infos(
      bool expect_multiple_isolation_infos) {
    expect_multiple_isolation_infos_ = expect_multiple_isolation_infos;
  }

 protected:
  int GetNextId(int min, int max) {
    EXPECT_FALSE(transaction_ids_.empty());
    int id = transaction_ids_.front();
    transaction_ids_.pop_front();
    EXPECT_GE(id, min);
    EXPECT_LE(id, max);
    return id;
  }

  DnsConfig config_;

  std::vector<std::unique_ptr<DnsSocketData>> socket_data_;

  base::circular_deque<int> transaction_ids_;
  std::unique_ptr<TestSocketFactory> socket_factory_;
  std::unique_ptr<TestURLRequestContext> request_context_;
  std::unique_ptr<ResolveContext> resolve_context_;
  scoped_refptr<DnsSession> session_;
  std::unique_ptr<DnsTransactionFactory> transaction_factory_;
  ResponseModifierCallback response_modifier_;
  DohJobMakerCallback doh_job_maker_;

  // Whether multiple IsolationInfos should be expected (due to there being
  // multiple RequestContexts in use).
  bool expect_multiple_isolation_infos_ = false;

  // IsolationInfo used by DoH requests. Populated on first DoH request, and
  // compared to IsolationInfo used by all subsequent requests, unless
  // |expect_multiple_isolation_infos_| is true.
  std::unique_ptr<IsolationInfo> isolation_info_;
};

static const char kTestCryptoHostName[] = "test.crypto";

// Response contains IP address: 142.250.72.196 for test.crypto.
static const uint8_t kTestCryptoResponseDatagram[] = {
    0x00, 0x00, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x74, 0x65, 0x73, 0x74, 0x06, 0x63, 0x72, 0x79, 0x70, 0x74, 0x6f,
    0x00, 0x00, 0x01, 0x00, 0x01, 0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x00,
    0x00, 0x00, 0xa2, 0x00, 0x04, 0x8e, 0xfa, 0x48, 0xc4};

static const char kTestEthHostName[] = "test.eth";

// Response contains IP address: 142.250.72.196 for test.eth.
static const uint8_t kTestEthResponseDatagram[] = {
    0x00, 0x00, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x04, 0x74, 0x65, 0x73, 0x74, 0x03, 0x65, 0x74, 0x68, 0x00,
    0x00, 0x01, 0x00, 0x01, 0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x00,
    0x00, 0x00, 0xa2, 0x00, 0x04, 0x8e, 0xfa, 0x48, 0xc4};

class BraveDnsTransactionTest : public DnsTransactionTestBase,
                                public WithTaskEnvironment {
 public:
  BraveDnsTransactionTest() = default;

  ~BraveDnsTransactionTest() override = default;

  void BraveConfigureDohServers(bool user_doh_server) {
    GURL url(decentralized_dns::kUnstoppableDomainsDoHResolver);
    URLRequestFilter* filter = URLRequestFilter::GetInstance();
    filter->AddHostnameInterceptor(url.scheme(), url.host(),
                                   std::make_unique<DohJobInterceptor>(this));
    config_.dns_over_https_servers.push_back(
        *DnsOverHttpsServerConfig::FromString(
            decentralized_dns::kUnstoppableDomainsDoHResolver));

    url = GURL(decentralized_dns::kENSDoHResolver);
    filter->AddHostnameInterceptor(url.scheme(), url.host(),
                                   std::make_unique<DohJobInterceptor>(this));
    config_.dns_over_https_servers.push_back(
        *DnsOverHttpsServerConfig::FromString(
            decentralized_dns::kENSDoHResolver));

    if (user_doh_server) {
      url = GURL("https://test.com/dns-query");
      filter->AddHostnameInterceptor(url.scheme(), url.host(),
                                     std::make_unique<DohJobInterceptor>(this));
      config_.dns_over_https_servers.push_back(
          *DnsOverHttpsServerConfig::FromString(url.spec()));
    }

    ConfigureFactory();
    for (size_t server_index = 0;
         server_index < config_.dns_over_https_servers.size(); ++server_index) {
      resolve_context_->RecordServerSuccess(
          server_index, true /* is_doh_server */, session_.get());
    }
  }
};

TEST_F(BraveDnsTransactionTest,
       SkipDecentralizedDNSResolversForNonTargetTLDsWithoutUserDoHServer) {
  BraveConfigureDohServers(false);
  EXPECT_TRUE(resolve_context_->GetDohServerAvailability(
      0u /* doh_server_index */, session_.get()));
  EXPECT_TRUE(resolve_context_->GetDohServerAvailability(
      1u /* doh_server_index */, session_.get()));
  TransactionHelper helper0(ERR_BLOCKED_BY_CLIENT);
  helper0.StartTransaction(transaction_factory_.get(), kT0HostName, kT0Qtype,
                           true /* secure */, resolve_context_.get());
  helper0.RunUntilComplete();
}

TEST_F(BraveDnsTransactionTest,
       SkipDecentralizedDNSResolversForNonTargetTLDsWithUserDoHServer) {
  BraveConfigureDohServers(true);
  AddQueryAndResponse(0, kT0HostName, kT0Qtype, kT0ResponseDatagram,
                      base::size(kT0ResponseDatagram), SYNCHRONOUS,
                      Transport::HTTPS, nullptr /* opt_rdata */,
                      DnsQuery::PaddingStrategy::BLOCK_LENGTH_128,
                      false /* enqueue_transaction_id */);
  TransactionHelper helper0(kT0RecordCount);
  helper0.StartTransaction(transaction_factory_.get(), kT0HostName, kT0Qtype,
                           true /* secure */, resolve_context_.get());
  helper0.RunUntilComplete();
}

TEST_F(BraveDnsTransactionTest,
       UseUDResolverForCryptoDomainsWithoutUserDoHServer) {
  BraveConfigureDohServers(false);
  AddQueryAndResponse(
      0, kTestCryptoHostName, dns_protocol::kTypeA, kTestCryptoResponseDatagram,
      base::size(kTestCryptoResponseDatagram), SYNCHRONOUS, Transport::HTTPS,
      nullptr /* opt_rdata */, DnsQuery::PaddingStrategy::BLOCK_LENGTH_128,
      false /* enqueue_transaction_id */);
  TransactionHelper helper0(1);
  helper0.StartTransaction(transaction_factory_.get(), kTestCryptoHostName,
                           dns_protocol::kTypeA, true /* secure */,
                           resolve_context_.get());
  helper0.RunUntilComplete();
}

TEST_F(BraveDnsTransactionTest,
       UseUDResolverForCryptoDomainsWithUserDoHServer) {
  BraveConfigureDohServers(true);
  AddQueryAndResponse(
      0, kTestCryptoHostName, dns_protocol::kTypeA, kTestCryptoResponseDatagram,
      base::size(kTestCryptoResponseDatagram), SYNCHRONOUS, Transport::HTTPS,
      nullptr /* opt_rdata */, DnsQuery::PaddingStrategy::BLOCK_LENGTH_128,
      false /* enqueue_transaction_id */);
  TransactionHelper helper0(1);
  helper0.StartTransaction(transaction_factory_.get(), kTestCryptoHostName,
                           dns_protocol::kTypeA, true /* secure */,
                           resolve_context_.get());
  helper0.RunUntilComplete();
}

TEST_F(BraveDnsTransactionTest,
       UseENSResolverForEthDomainsWithoutUserDoHServer) {
  BraveConfigureDohServers(false);
  AddQueryAndResponse(
      0, kTestEthHostName, dns_protocol::kTypeA, kTestEthResponseDatagram,
      base::size(kTestEthResponseDatagram), SYNCHRONOUS, Transport::HTTPS,
      nullptr /* opt_rdata */, DnsQuery::PaddingStrategy::BLOCK_LENGTH_128,
      false /* enqueue_transaction_id */);
  TransactionHelper helper0(1);
  helper0.StartTransaction(transaction_factory_.get(), kTestEthHostName,
                           dns_protocol::kTypeA, true /* secure */,
                           resolve_context_.get());
  helper0.RunUntilComplete();
}

TEST_F(BraveDnsTransactionTest, UseENSResolverForEthDomainsWithUserDoHServer) {
  BraveConfigureDohServers(false);
  AddQueryAndResponse(
      0, kTestEthHostName, dns_protocol::kTypeA, kTestEthResponseDatagram,
      base::size(kTestEthResponseDatagram), SYNCHRONOUS, Transport::HTTPS,
      nullptr /* opt_rdata */, DnsQuery::PaddingStrategy::BLOCK_LENGTH_128,
      false /* enqueue_transaction_id */);
  TransactionHelper helper0(1);
  helper0.StartTransaction(transaction_factory_.get(), kTestEthHostName,
                           dns_protocol::kTypeA, true /* secure */,
                           resolve_context_.get());
  helper0.RunUntilComplete();
}

}  // namespace

}  // namespace net
