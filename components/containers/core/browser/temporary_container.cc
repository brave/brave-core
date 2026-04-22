// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/temporary_container.h"

#include <array>
#include <string>

#include "base/check.h"
#include "base/rand_util.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/uuid.h"
#include "brave/components/containers/core/mojom/containers.mojom-shared.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "brave/third_party/bip39wally-core-native/include/wally_bip39.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_manager.h"

namespace containers {
namespace {

// Generates a random temporary container name using first two BIP39 mnemonic
// words generated from random entropy. The first word is title cased.
std::string GenerateTemporaryContainerName() {
  std::array<uint8_t, 16> entropy;
  base::RandBytes(entropy);
  char* words_cstr = nullptr;
  CHECK_EQ(bip39_mnemonic_from_bytes(nullptr, entropy.data(), entropy.size(),
                                     &words_cstr),
           WALLY_OK);
  std::string mnemonic(words_cstr);
  wally_free_string(words_cstr);

  const auto words = base::SplitStringPiece(
      mnemonic, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  CHECK_GE(words.size(), 2u);
  std::string word_0(words[0]);
  // Title case the first word.
  CHECK(!word_0.empty());
  word_0[0] = base::ToUpperASCII(word_0[0]);
  return base::StrCat({word_0, " ", words[1]});
}

// Picks a random container icon from the available icons.
mojom::Icon PickTemporaryContainerIcon() {
  static constexpr auto kIconIds = std::to_array({
      mojom::Icon::kPersonal,
      mojom::Icon::kWork,
      mojom::Icon::kShopping,
      mojom::Icon::kSocial,
      mojom::Icon::kEvents,
      mojom::Icon::kBanking,
      mojom::Icon::kStar,
      mojom::Icon::kTravel,
      mojom::Icon::kSchool,
      mojom::Icon::kPrivate,
      mojom::Icon::kMessaging,
  });
  return base::RandomChoice(kIconIds);
}

// Picks a random container background color from the available colors.
SkColor PickTemporaryContainerBackground() {
  const auto* color_provider =
      ui::ColorProviderManager::Get().GetColorProviderFor(
          ui::ColorProviderKey());
  CHECK(color_provider);

  // Keep in sync with
  // brave/browser/resources/settings/brave_content_page/background_colors.ts.
  static constexpr auto kBackgroundColorIds = std::to_array({
      nala::kColorPrimitiveRed60,
      nala::kColorPrimitiveOrange60,
      nala::kColorPrimitiveYellow60,
      nala::kColorPrimitiveGreen60,
      nala::kColorPrimitiveTeal60,
      nala::kColorPrimitiveBlue60,
      nala::kColorPrimitivePurple60,
      nala::kColorPrimitivePink60,
  });
  return color_provider->GetColor(base::RandomChoice(kBackgroundColorIds));
}

}  // namespace

bool IsTemporaryContainerId(std::string_view container_id) {
  // Temporary container ids are prefixed with "t-" and should have the actual
  // id after the prefix.
  return container_id.size() > kTemporaryContainerIdPrefix.size() &&
         container_id.starts_with(kTemporaryContainerIdPrefix);
}

mojom::ContainerPtr CreateTemporaryContainer() {
  return mojom::Container::New(
      base::StrCat({kTemporaryContainerIdPrefix,
                    base::Uuid::GenerateRandomV4().AsLowercaseString()}),
      GenerateTemporaryContainerName(), PickTemporaryContainerIcon(),
      PickTemporaryContainerBackground());
}

}  // namespace containers
