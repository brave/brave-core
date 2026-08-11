#ifndef BRAVE_BNES_BNS_SCHEME_HANDLER_H_
#define BRAVE_BNES_BNS_SCHEME_HANDLER_H_

#include <string_view>

#include "url/gurl.h"

namespace bns {

bool IsBnesScheme(const GURL& url);

}  // namespace bns

#endif  // BRAVE_BNES_BNS_SCHEME_HANDLER_H_
