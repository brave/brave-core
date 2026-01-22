#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_MOJO_SECURE_STRING_MOJO_TRAITS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_MOJO_SECURE_STRING_MOJO_TRAITS_H_

#include "base/containers/span.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-shared.h"  // IWYU pragma: export
#include "crypto/process_bound_string.h"
#include "mojo/public/cpp/base/big_buffer.h"

namespace mojo {

template <>
struct StructTraits<brave_wallet::mojom::SecureBufferDataView,
                    crypto::SecureString> {
  static ::mojo_base::BigBuffer data(const crypto::SecureString& input) {
    return ::mojo_base::BigBuffer(base::as_byte_span(input));
  }

  static bool Read(brave_wallet::mojom::SecureBufferDataView data,
                   crypto::SecureString* out) {
    ::mojo_base::mojom::BigBufferDataView data_view;

    data.GetDataDataView(&data_view);
    if (!data_view.is_bytes()) {
      return false;
    }

    mojo::ArrayDataView<uint8_t> bytes;
    data_view.GetBytesDataView(&bytes);

    out->assign(bytes.begin(), bytes.end());
    return true;
  }
};

}  // namespace mojo

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_MOJO_SECURE_STRING_MOJO_TRAITS_H_
