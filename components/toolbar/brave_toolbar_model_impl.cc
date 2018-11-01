// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/toolbar/brave_toolbar_model_impl.h"

#include "brave/common/extensions/extension_constants.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/toolbar/constants.h"
#include "components/toolbar/toolbar_model_impl.h"

using namespace brave_toolbar;

namespace {

const base::string16 original_scheme_part =
                                  base::ASCIIToUTF16(kOriginalInternalUIScheme);
const base::string16 replacement_scheme_part =
                                  base::ASCIIToUTF16(kInternalUIScheme);

const base::string16 pdfjs_url_prefix =
                                  base::ASCIIToUTF16("chrome-extension://") +
                                  base::ASCIIToUTF16(pdfjs_extension_id) +
                                  base::ASCIIToUTF16("/");
}

base::string16 BraveToolbarModelImpl::GetURLForDisplay() const {
  base::string16 formatted_text = ToolbarModelImpl::GetURLForDisplay();

  const GURL url(GetURL());
  // Only replace chrome:// with brave:// if scheme is "chrome" and
  // it has not been stripped from the display text
  if (url.SchemeIs(kOriginalInternalUIScheme) &&
      base::StartsWith(formatted_text, original_scheme_part,
                      base::CompareCase::INSENSITIVE_ASCII)) {
    formatted_text.replace(0, original_scheme_part.length(),
                            replacement_scheme_part);
  }
  else if (base::StartsWith(formatted_text, pdfjs_url_prefix,
                       base::CompareCase::SENSITIVE)) {
    formatted_text.erase(0, pdfjs_url_prefix.length());
  }
  return formatted_text;
}
