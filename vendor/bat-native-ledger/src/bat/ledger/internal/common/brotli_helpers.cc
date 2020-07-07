/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/common/brotli_helpers.h"

#include <vector>

#include "third_party/brotli/include/brotli/decode.h"

namespace {

class BrotliStreamDecoder {
 public:
  explicit BrotliStreamDecoder(size_t buffer_size) {
    brotli_state_ = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
    out_vector_.reserve(buffer_size);
  }

  BrotliStreamDecoder(const BrotliStreamDecoder&) = delete;
  BrotliStreamDecoder& operator=(const BrotliStreamDecoder&) = delete;

  ~BrotliStreamDecoder() {
    BrotliDecoderDestroyInstance(brotli_state_);
  }

  enum class Result {
    Done = 0,
    InputRequired,
    Error,
  };

  template<typename F>
  Result Decode(const uint8_t* input_buffer, size_t input_length, F callback) {
    if (!input_buffer || input_length == 0) {
      return Result::Error;
    }

    uint8_t* output_buffer = out_vector_.data();
    size_t output_length = out_vector_.capacity();

    for (;;) {
      auto brotli_result = BrotliDecoderDecompressStream(
          brotli_state_,
          &input_length,
          &input_buffer,
          &output_length,
          &output_buffer,
          nullptr);

      switch (brotli_result) {
        case BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT: {
          callback(out_vector_.data(), out_vector_.capacity() - output_length);
          output_buffer = out_vector_.data();
          output_length = out_vector_.capacity();
          break;
        }
        case BROTLI_DECODER_RESULT_SUCCESS: {
          callback(out_vector_.data(), out_vector_.capacity() - output_length);
          return Result::Done;
        }
        case BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT: {
          return Result::InputRequired;
        }
        default: {
          return Result::Error;
        }
      }
    }
  }

 private:
  BrotliDecoderState* brotli_state_;
  std::vector<uint8_t> out_vector_;
};

}  // namespace

namespace braveledger_helpers {

bool DecodeBrotliString(
    base::StringPiece input,
    size_t uncompressed_size,
    std::string* output) {
  DCHECK(output);
  if (input.empty()) {
    return false;
  }

  output->resize(uncompressed_size);
  auto result = BrotliDecoderDecompress(
      input.size(),
      reinterpret_cast<const uint8_t*>(input.data()),
      &uncompressed_size,
      reinterpret_cast<uint8_t*>(const_cast<char*>(output->data())));

  return result == BROTLI_DECODER_RESULT_SUCCESS;
}

bool DecodeBrotliStringWithBuffer(
    base::StringPiece input,
    size_t buffer_size,
    std::string* output) {
  DCHECK(output);
  if (input.empty()) {
    return false;
  }

  output->resize(0);
  BrotliStreamDecoder decoder(buffer_size);
  auto result = decoder.Decode(
      reinterpret_cast<const uint8_t*>(input.data()),
      input.size(),
      [output](uint8_t* buffer, size_t length) {
        output->append(reinterpret_cast<char*>(buffer), length);
      });

  return result == BrotliStreamDecoder::Result::Done;
}

}  // namespace braveledger_helpers
