// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_SANITIZED_IMAGE_SOURCE_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_SANITIZED_IMAGE_SOURCE_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "components/signin/public/identity_manager/access_token_info.h"
#include "ios/web/public/webui/url_data_source_ios.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "services/data_decoder/public/cpp/decode_image.h"
#include "services/data_decoder/public/mojom/image_decoder.mojom.h"
#include "url/gurl.h"

class ProfileIOS;
class SkBitmap;

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace signin {
class IdentityManager;
}  // namespace signin

class SanitizedImageSource : public web::URLDataSourceIOS {
 public:
  using DecodeImageCallback = data_decoder::DecodeImageCallback;
  using DecodeAnimationCallback =
      data_decoder::mojom::ImageDecoder::DecodeAnimationCallback;

  // A delegate class that is faked out for testing purposes.
  class DataDecoderDelegate {
   public:
    DataDecoderDelegate() = default;
    virtual ~DataDecoderDelegate() = default;

    virtual void DecodeImage(const std::string& data,
                             DecodeImageCallback callback);

    virtual void DecodeAnimation(const std::string& data,
                                 DecodeAnimationCallback callback);

   private:
    // The instance of the Data Decoder used by this DataDecoderDelegate to
    // perform any image decoding operations. The underlying service instance is
    // started lazily when needed and torn down when not in use.
    data_decoder::DataDecoder data_decoder_;
  };

  explicit SanitizedImageSource(ProfileIOS* profile);
  // This constructor lets us pass mock dependencies for testing.
  SanitizedImageSource(
      ProfileIOS* profile,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      std::unique_ptr<DataDecoderDelegate> delegate);
  SanitizedImageSource(const SanitizedImageSource&) = delete;
  SanitizedImageSource& operator=(const SanitizedImageSource&) = delete;
  ~SanitizedImageSource() override;

  // web::URLDataSourceIOS:
  std::string GetSource() const override;
  void StartDataRequest(
      std::string_view path,
      web::URLDataSourceIOS::GotDataCallback callback) override;
  std::string GetMimeType(std::string_view path) const override;
  bool ShouldReplaceExistingSource() const override;

 private:
  struct RequestAttributes {
    enum EncodeType {
      kPng = 0,
      kWebP = 1,
    };

    RequestAttributes();
    RequestAttributes(const RequestAttributes&);
    ~RequestAttributes();

    GURL image_url = GURL();
    bool static_encode = false;
    EncodeType encode_type = EncodeType::kPng;
    std::optional<signin::AccessTokenInfo> access_token_info;
  };

  void StartImageDownload(RequestAttributes request_attributes,
                          web::URLDataSourceIOS::GotDataCallback callback);
  void OnImageLoaded(std::unique_ptr<network::SimpleURLLoader> loader,
                     RequestAttributes request_attributes,
                     web::URLDataSourceIOS::GotDataCallback callback,
                     std::optional<std::string> body);
  void OnAnimationDecoded(
      RequestAttributes request_attributes,
      web::URLDataSourceIOS::GotDataCallback callback,
      std::vector<data_decoder::mojom::AnimationFramePtr> mojo_frames);

  void EncodeAndReplyStaticImage(
      RequestAttributes request_attributes,
      web::URLDataSourceIOS::GotDataCallback callback,
      const SkBitmap& bitmap);
  void EncodeAndReplyAnimatedImage(
      web::URLDataSourceIOS::GotDataCallback callback,
      std::vector<data_decoder::mojom::AnimationFramePtr> mojo_frames);

  // Owned by `IdentityManagerFactory` or `IdentityTestEnvironment`.
  raw_ptr<signin::IdentityManager> identity_manager_;

  const scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  std::unique_ptr<DataDecoderDelegate> data_decoder_delegate_;
  std::string pcdn_domain_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<SanitizedImageSource> weak_ptr_factory_{this};
};

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_SANITIZED_IMAGE_SOURCE_H_
