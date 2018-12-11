/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "confirmations_impl.h"
#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/catalog_issuers_info.h"
#include "logging.h"
#include "static_values.h"

static const char* BRAVE_AD_SERVER = "ads-serve.bravesoftware.com";
static int BRAVE_AD_SERVER_PORT = 80;
extern int count;
extern std::string happy_data; 
extern int happy_status;

void OnBegin( const happyhttp::Response* r, void* userdata )
{
  // printf("BEGIN (%d %s)\n", r->getstatus(), r->getreason() );
  count = 0;
  happy_data = "";
  happy_status = r->getstatus();
}

void OnData( const happyhttp::Response* r, void* userdata, const unsigned char* data, int n )
{
  //fwrite( data,1,n, stdout );
  happy_data.append((char *)data, (size_t)n);
  count += n;
}

void OnComplete( const happyhttp::Response* r, void* userdata )
{
  happy_status = r->getstatus();
  // printf("END (%d %s)\n", r->getstatus(), r->getreason() );
  // printf("COMPLETE (%d bytes)\n", count );
}

namespace confirmations {

ConfirmationsImpl::ConfirmationsImpl(
      ConfirmationsClient* confirmations_client) :
    is_initialized_(false),
    confirmations_client_(confirmations_client) {
}

ConfirmationsImpl::~ConfirmationsImpl() = default;

void ConfirmationsImpl::Initialize() {
  if (is_initialized_) {
    BLOG(WARNING) << "Already initialized";
    return;
  }

  is_initialized_ = true;

  BLOG(INFO) << BRAVE_AD_SERVER;
  BLOG(INFO) << BRAVE_AD_SERVER_PORT;
  BLOG(INFO) << "Hello";
}

void ConfirmationsImpl::OnCatalogIssuersChanged(
    const CatalogIssuersInfo& info) {
}

void ConfirmationsImpl::OnTimer(const uint32_t timer_id) {
}

}  // namespace confirmations
