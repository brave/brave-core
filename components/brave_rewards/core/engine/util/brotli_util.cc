/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/engine/util/brotli_util.h"

#include <memory>
#include <string_view>
#include <vector>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/functional/function_ref.h"
#include "base/strings/string_view_util.h"
#include "third_party/brotli/include/brotli/decode.h"

namespace {

class BrotliStreamDecoder {
 public:
  explicit BrotliStreamDecoder(size_t buffer_size)
      : decoder_(BrotliDecoderCreateInstance(nullptr, nullptr, nullptr),
                 BrotliDecoderDestroyInstance) {
    out_vector_.resize(buffer_size);
  }

  BrotliStreamDecoder(const BrotliStreamDecoder&) = delete;
  BrotliStreamDecoder& operator=(const BrotliStreamDecoder&) = delete;

  enum class Result {
    Done = 0,
    InputRequired,
    Error,
  };

  Result Decode(base::span<const uint8_t> input,
                base::FunctionRef<void(base::span<const uint8_t>)> callback) {
    if (input.empty()) {
      return Result::Error;
    }

    size_t input_length = input.size();
    const uint8_t* input_buffer = input.data();

    uint8_t* output_buffer = out_vector_.data();
    size_t output_length = out_vector_.size();

    for (;;) {
      auto brotli_result = BrotliDecoderDecompressStream(
          decoder_.get(), &input_length, &input_buffer, &output_length,
          &output_buffer, nullptr);

      switch (brotli_result) {
        case BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT: {
          callback(base::span(out_vector_)
                       .first(out_vector_.size() - output_length));
          output_buffer = out_vector_.data();
          output_length = out_vector_.size();
          break;
        }
        case BROTLI_DECODER_RESULT_SUCCESS: {
          callback(base::span(out_vector_)
                       .first(out_vector_.size() - output_length));
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
  std::unique_ptr<BrotliDecoderState, decltype(&BrotliDecoderDestroyInstance)>
      decoder_;
  std::vector<uint8_t> out_vector_;
};

}  // namespace

namespace brave_rewards::internal::util {

bool DecodeBrotliString(std::string_view input, base::span<uint8_t> output) {
  if (input.empty()) {
    return false;
  }

  auto input_data = base::as_byte_span(input);
  size_t output_length = output.size();
  auto result = BrotliDecoderDecompress(input_data.size(), input_data.data(),
                                        &output_length, output.data());

  return result == BROTLI_DECODER_RESULT_SUCCESS;
}

bool DecodeBrotliStringWithBuffer(std::string_view input,
                                  size_t buffer_size,
                                  std::string* output) {
  DCHECK(output);
  if (input.empty()) {
    return false;
  }

  output->clear();
  BrotliStreamDecoder decoder(buffer_size);
  auto result = decoder.Decode(base::as_byte_span(input),
                               [output](base::span<const uint8_t> data) {
                                 output->append(base::as_string_view(data));
                               });

  return result == BrotliStreamDecoder::Result::Done;
}

}  // namespace brave_rewards::internal::util
