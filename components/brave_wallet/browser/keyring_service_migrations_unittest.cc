/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/password_encryptor.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJsonDict;
using testing::_;

namespace brave_wallet {

namespace {
constexpr char kPassword[] = "brave123!";

// "brave.wallet.keyrings" pref for 1.43.93 before encryptor's pbkdf2 iterations
// were changed from 100000 to 310000.
constexpr char kLegacyKeyringsDict143[] = R"(
    {
      "default": {
          "account_metas": {
            "m/44'/60'/0'/0/0": {
                "account_address": "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db",
                "account_name": "Account 1"
            },
            "m/44'/60'/0'/0/1": {
                "account_address": "0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0",
                "account_name": "Account 2"
            }
          },
          "backup_complete": true,
          "encrypted_mnemonic": "u0UHv7FTULB+1ydfVbQwhZ3d7qMwCbcRig0Rs++P+0xK8DOP1cUlDyssdIr2WpEXttfm5l/FQWY5mlg/mv+CmIpMAgVAL0DY3TabZvBaS9gc5ovEPS3MgGPT",
          "hardware": {},
          "imported_accounts": [ {
            "account_address": "0xDc06aE500aD5ebc5972A0D8Ada4733006E905976",
            "account_name": "Imported ETH",
            "coin_type": 60,
            "encrypted_private_key": "WIDGcsmoedOwbzQHVGLj574NPCu1i48Leeu5LhSZcPRGg7vSr3/jjbPC1EUDMHI4"
          } ],
          "legacy_brave_wallet": false,
          "password_encryptor_nonce": "72wD2tPBarMRIO5m",
          "password_encryptor_salt": "YuKtz9T1tQJB13BNeCqtSbBogByb8qyjkhc8/OBKLME=",
          "selected_account": "0xDc06aE500aD5ebc5972A0D8Ada4733006E905976"
      },
      "filecoin": {
          "account_metas": {
            "m/44'/461'/0'/0/0": {
                "account_address": "f1qjidlytseoouzfhsgzczf3ettbhuaezorczeava",
                "account_name": "Filecoin Account 1"
            }
          },
          "encrypted_mnemonic": "Ktr5hSxVm+ZL/DDSyGHNzN00Ygp+MV3uKYY+DTaNj5uJtcxqSLc4GYsKaTxOicj1imqyZQE53ZVnRuSpl0G0Q7mtZanZ9Lm6JZoHq9cVrd+0+B+8ERNjX2S7",
          "hardware": {},
          "imported_accounts": [ {
            "account_address": "f1syhomjrwhjmavadwmrofjpiocb6r72h4qoy7ucq",
            "account_name": "Imported FIL",
            "coin_type": 461,
            "encrypted_private_key": "eJcvcbMrsqwyByr+OY1mz5ccSRv3EgmjJrZ70YNdD+XHehIk7eyKL9kipptUsgcW"
          } ],
          "legacy_brave_wallet": false,
          "password_encryptor_nonce": "X9dV7rwaQBAcTXqR",
          "password_encryptor_salt": "cPyPCUcKcBrdgP8w9L6EgThFpaAaZmcLH+J3Oo7VSUc=",
          "selected_account": "f1syhomjrwhjmavadwmrofjpiocb6r72h4qoy7ucq"
      },
      "filecoin_testnet": {
          "hardware": {},
          "password_encryptor_salt": "9GxRVuwCF5A613jswL7vwsH9Svsxkf0+MBUvgcNBZrA="
      },
      "solana": {
          "account_metas": {
            "m/44'/501'/0'/0'": {
                "account_address": "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
                "account_name": "Solana Account 1"
            }
          },
          "encrypted_mnemonic": "FP10piEBXuHXJiQ/a8+w1VoEGSqAcOA63WV209ejhXrR1wG9e8T9N011PeilvI8+dhfXtmGl2qjSOR9z+1Dzvu3cK+ikcJBBK1KdLz51iAnqGUHeEMf9Fdad",
          "hardware": {},
          "imported_accounts": [ {
            "account_address": "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ",
            "account_name": "Imported Solana",
            "coin_type": 501,
            "encrypted_private_key": "D7inkYggdcfZgSET+BGXrbO7Re96REw9OjuHG9P4jD5XQH6uc0r4m4AEiw9pKvltBint1ErK1m1VisE/00Sp7bMaxGv/oj+ugXVqWyg00tw="
          } ],
          "legacy_brave_wallet": false,
          "password_encryptor_nonce": "Eq9en95V8OyJDM+P",
          "password_encryptor_salt": "9TKOHitXfgNgS2oIRn60QkZFiXB4sXLyphTTBenuga8=",
          "selected_account": "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ"
      }
    }
  )";

constexpr char kUpgradedKeyringsDict143[] = R"(
  {
    "bitcoin_84": {},
    "bitcoin_84_test": {},
    "default": {
        "account_metas": [ {
          "account_address": "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db",
          "account_index": "0",
          "account_name": "Account 1"
        }, {
          "account_address": "0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0",
          "account_index": "1",
          "account_name": "Account 2"
        } ],
        "hardware": {},
        "imported_accounts": [ {
          "account_address": "0xDc06aE500aD5ebc5972A0D8Ada4733006E905976",
          "account_name": "Imported ETH",
          "encrypted_private_key": {
              "ciphertext": "UCCszJGN5nwgrzaTrgUxygc9v/A68831l90g7ZtzGfXT2sfytjm5ka0q10UeXaBF",
              "nonce": "AgICAgICAgICAgIC"
          }
        } ]
    },
    "filecoin": {
        "account_metas": [ {
          "account_address": "f1qjidlytseoouzfhsgzczf3ettbhuaezorczeava",
          "account_index": "0",
          "account_name": "Filecoin Account 1"
        } ],
        "hardware": {},
        "imported_accounts": [ {
          "account_address": "f1syhomjrwhjmavadwmrofjpiocb6r72h4qoy7ucq",
          "account_name": "Imported FIL",
          "encrypted_private_key": {
              "ciphertext": "eI2zyh1tAPt3qlKHu1ZDiUsMe9dQ1g8iyi+4X1GWlPioq45+IPTeXzWu4MMMGIil",
              "nonce": "AwMDAwMDAwMDAwMD"
          }
        } ]
    },
    "filecoin_testnet": {
        "hardware": {}
    },
    "solana": {
        "account_metas": [ {
          "account_address": "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
          "account_index": "0",
          "account_name": "Solana Account 1"
        } ],
        "hardware": {},
        "imported_accounts": [ {
          "account_address": "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ",
          "account_name": "Imported Solana",
          "encrypted_private_key": {
              "ciphertext": "E8EzxPcuyFYoaN1zTSb/CakdNATAd9QTLLjazzb4JwlxnOhuMKs9wSR7h1tR1hV9jTXp3ZWMTmnA9Wx9rynHBEkz0iKudBNPziu5FdmJprk=",
              "nonce": "BAQEBAQEBAQEBAQE"
          }
        } ]
    },
    "zcash_mainnet": {},
    "zcash_testnet": {}
  })";

// "brave.wallet.keyrings" pref for 1.66.95 before encrypted mnemonic was
// extracted to a separate pref.
constexpr char kLegacyKeyringsDict166[] = R"(
  {
    "bitcoin_84": {
      "account_metas": [
        {
          "account_address": "bc1qfcr9eaxuqsq2qelaq06ps7jt7ry8zcutsfdd32",
          "account_index": "0",
          "account_name": "Bitcoin Account 1"
        }
      ],
      "encrypted_mnemonic": "QWgqiMpj3xeidKdJr3GYCnBxhKjgwhHhLnbyPcNHMZ+MKbIXncsgIeyd4VIIl3Ean25c6iSVB/HIS14zmS2zc2eZOQSpIR4I37J925trpIH4Vyx2/9mk+qPo",
      "legacy_brave_wallet": false,
      "password_encryptor_nonce": "SjN4UjXrXQ4p7CSC",
      "password_encryptor_salt": "ugpNnrfyM77/0hgtHcYxcmxYrSvHz2b/d6WrQr4h4EE="
    },
    "bitcoin_84_test": {
      "account_metas": [
        {
          "account_address": "tb1qn8j468j6xhuz4zr9256npmvc0kaw02ytjjcwqw",
          "account_index": "0",
          "account_name": "Bitcoin Account 2"
        }
      ],
      "encrypted_mnemonic": "0mKMQO/Mo9wDSahQ4MRvhEbKXhix6v1E5VbpFK59Rwe+x7aGu4ydgaw1lVss8OvU7yLiB9l64/iVeyai3vzlgBq14msHGYqa2clef1tiTuEU1Qf5SpQVLIem",
      "legacy_brave_wallet": false,
      "password_encryptor_nonce": "bvJKTW2d2K1JcCP4",
      "password_encryptor_salt": "nk3wtIyQLQNR6Lig41CxsKhhv1ffspGwKQBjMLlGhqQ="
    },
    "default": {
      "account_metas": [
        {
          "account_address": "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db",
          "account_index": "0",
          "account_name": "Account 1"
        },
        {
          "account_address": "0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0",
          "account_index": "1",
          "account_name": "Account 2"
        }
      ],
      "backup_complete": true,
      "encrypted_mnemonic": "KXS1xuc8m4mlwtsyv6FtOdVhyBIsGKMin+cGryfn3MCrB333x/v/eL7qIkAXHBe3MMJc2TA0PusBA/8mGp1+8dNRPr5yKB9uDNxuheOmxItF1nbqNL4HR/eQ",
      "imported_accounts": [
        {
          "account_address": "0xDc06aE500aD5ebc5972A0D8Ada4733006E905976",
          "account_name": "Imported ETH",
          "encrypted_private_key": "MYw14aZynnTwS3faRhiIjTcObm+IBVF8LfVBqKa5VHn+j68+CuyMYJrs1d21ZmJy"
        }
      ],
      "legacy_brave_wallet": false,
      "password_encryptor_nonce": "Qme2oq1dHs/l0and",
      "password_encryptor_salt": "FcB57vEUkrDJ9I0B8Ppug6O4zl27M/sAnDRkqjcaSG8="
    },
    "filecoin": {
      "account_metas": [
        {
          "account_address": "f1qjidlytseoouzfhsgzczf3ettbhuaezorczeava",
          "account_index": "0",
          "account_name": "Filecoin Account 1"
        }
      ],
      "encrypted_mnemonic": "AksNejlKAZ3dPuD434HhG5fF69hABrm73mzQrHLSr4UhlsDWeYduEdmlqCJHXt89mobqc4xu/UaPfhBjoMNgnCN1CzsEiUs2f81NWGSVxoO4UjkIxHhxmJZc",
      "imported_accounts": [
        {
          "account_address": "f1syhomjrwhjmavadwmrofjpiocb6r72h4qoy7ucq",
          "account_name": "Imported fil",
          "encrypted_private_key": "sUDYfe6ozHkaN6CqAe/oe04FI/owNFkf4tTFxCOVPxAnNjRpidW/ucpjg+EJRHlv"
        }
      ],
      "legacy_brave_wallet": false,
      "password_encryptor_nonce": "iXAdELqFcyVf7Nav",
      "password_encryptor_salt": "9qptfdhEbTTJJEiueqnuXCQ3IPcYS24GsL8okZfX904="
    },
    "filecoin_testnet": {
      "account_metas": [
        {
          "account_address": "t1dca7adhz5lbvin5n3qlw67munu6xhn5fpb77nly",
          "account_index": "0",
          "account_name": "Filecoin Account 2"
        }
      ],
      "encrypted_mnemonic": "QPmypf9S/86WZIxhnlmShtTnmkGVsrcHgCoJ8U0MYJ4MqGKqn6+k4vLF6JAeEkjX41hwNk5Wkw25yt5bygDuTQmiLErKsoKggbQ8OLy0FvOlZGTMsEFzwZju",
      "legacy_brave_wallet": false,
      "password_encryptor_nonce": "KljHeM1oY9DZh7d8",
      "password_encryptor_salt": "d0N7AXYfVsiLoLqwaZVOj99Ihuc/pjUT9hrNrB9gjdg="
    },
    "solana": {
      "account_metas": [
        {
          "account_address": "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
          "account_index": "0",
          "account_name": "Solana Account 1"
        }
      ],
      "encrypted_mnemonic": "esJ8U1hYQEdKwlLuylHQk36G5qamyHCWSfTQ30lRGw3beI7hA6lZrRWjxifLKavWvcA200jAivfggB+i7mMYL0Mhl1IH/b2lt6NL2JlvXw7zVmwtHxHleUyY",
      "imported_accounts": [
        {
          "account_address": "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ",
          "account_name": "Imported Sol",
          "encrypted_private_key": "VPQm19YOYt+c1taPJg3gHuZjD4csf2IPqMwsrlW2jC5X3VQRFAXWyD1Q9fHlM2bwtWqUYpT4MVrB4b3dHwXlWqIkdU8JI/Qxkym6gIv31u0="
        }
      ],
      "legacy_brave_wallet": false,
      "password_encryptor_nonce": "tPt2YqFDNeHkZl+I",
      "password_encryptor_salt": "f9NvfnHXuIt+xIhc8miqsa+You8k++90upPyADH7BvA="
    },
    "zcash_mainnet": {
      "account_metas": [
        {
          "account_address": "t1gZE6cHk9EUKachqqoYDjXVNmnjuSqJooZ",
          "account_index": "0",
          "account_name": "ZCash Account 1",
          "bitcoin": {
            "next_change": "0",
            "next_receive": "1"
          }
        }
      ],
      "encrypted_mnemonic": "bEX5k0cvMA5cQ3LM9dqPB4aCD0J9KhNn0e2ghrwgNqKS1OKwnsimXUKQmlJ/MPcx1SeDYWhWtim3eMMK7TQk57ITwG/N+7tspnykYE3uyHbSz3nNsafXp/6E",
      "legacy_brave_wallet": false,
      "password_encryptor_nonce": "6iyU1ldLPiYr5R9/",
      "password_encryptor_salt": "Q4E9+RO7P1+Hd3gjn84JvguPeSd9tVKHbmubKScfYQ0="
    },
    "zcash_testnet": {
      "account_metas": [
        {
          "account_address": "tmMxQQXs8HXRTWggXUodFvZGwZgSZuAAq42",
          "account_index": "0",
          "account_name": "ZCash Account 2",
          "bitcoin": {
            "next_change": "0",
            "next_receive": "0"
          }
        }
      ],
      "encrypted_mnemonic": "k/uFUot14gPm1h5yG76yMwzVMM1oVfyUAG9Z3FH0tTsY1VVj9NFC/quUD9Sb/eTZnW5hEzDZxNLHcqlcm0xHvZOD6841Lzb6x8L0KwFUeaLqmbPNPsIWbMiF",
      "legacy_brave_wallet": false,
      "password_encryptor_nonce": "GITthKpVRNVRpkLW",
      "password_encryptor_salt": "aG31v+D3yF5uwHdUDyOvW+OHeXz7h9JX086mEbnye8c="
    }
  })";

constexpr char kUpgradedKeyringsDict166[] = R"(
  {
    "bitcoin_84": {
        "account_metas": [ {
          "account_address": "bc1qfcr9eaxuqsq2qelaq06ps7jt7ry8zcutsfdd32",
          "account_index": "0",
          "account_name": "Bitcoin Account 1"
        } ]
    },
    "bitcoin_84_test": {
        "account_metas": [ {
          "account_address": "tb1qn8j468j6xhuz4zr9256npmvc0kaw02ytjjcwqw",
          "account_index": "0",
          "account_name": "Bitcoin Account 2"
        } ]
    },
    "default": {
        "account_metas": [ {
          "account_address": "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db",
          "account_index": "0",
          "account_name": "Account 1"
        }, {
            "account_address": "0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0",
            "account_index": "1",
            "account_name": "Account 2"
        } ],
        "imported_accounts": [ {
          "account_address": "0xDc06aE500aD5ebc5972A0D8Ada4733006E905976",
          "account_name": "Imported ETH",
          "encrypted_private_key": {
              "ciphertext": "UCCszJGN5nwgrzaTrgUxygc9v/A68831l90g7ZtzGfXT2sfytjm5ka0q10UeXaBF",
              "nonce": "AgICAgICAgICAgIC"
          }
        } ]
    },
    "filecoin": {
        "account_metas": [ {
          "account_address": "f1qjidlytseoouzfhsgzczf3ettbhuaezorczeava",
          "account_index": "0",
          "account_name": "Filecoin Account 1"
        } ],
        "imported_accounts": [ {
          "account_address": "f1syhomjrwhjmavadwmrofjpiocb6r72h4qoy7ucq",
          "account_name": "Imported fil",
          "encrypted_private_key": {
              "ciphertext": "eI2zyh1tAPt3qlKHu1ZDiUsMe9dQ1g8iyi+4X1GWlPioq45+IPTeXzWu4MMMGIil",
              "nonce": "AwMDAwMDAwMDAwMD"
          }
        } ]
    },
    "filecoin_testnet": {
        "account_metas": [ {
          "account_address": "t1dca7adhz5lbvin5n3qlw67munu6xhn5fpb77nly",
          "account_index": "0",
          "account_name": "Filecoin Account 2"
        } ]
    },
    "solana": {
        "account_metas": [ {
          "account_address": "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
          "account_index": "0",
          "account_name": "Solana Account 1"
        } ],
        "imported_accounts": [ {
          "account_address": "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ",
          "account_name": "Imported Sol",
          "encrypted_private_key": {
              "ciphertext": "E8EzxPcuyFYoaN1zTSb/CakdNATAd9QTLLjazzb4JwlxnOhuMKs9wSR7h1tR1hV9jTXp3ZWMTmnA9Wx9rynHBEkz0iKudBNPziu5FdmJprk=",
              "nonce": "BAQEBAQEBAQEBAQE"
          }
        } ]
    },
    "zcash_mainnet": {
        "account_metas": [ {
          "account_address": "t1gZE6cHk9EUKachqqoYDjXVNmnjuSqJooZ",
          "account_index": "0",
          "account_name": "ZCash Account 1",
          "bitcoin": {
              "next_change": "0",
              "next_receive": "1"
          }
        } ]
    },
    "zcash_testnet": {
        "account_metas": [ {
          "account_address": "tmMxQQXs8HXRTWggXUodFvZGwZgSZuAAq42",
          "account_index": "0",
          "account_name": "ZCash Account 2",
          "bitcoin": {
              "next_change": "0",
              "next_receive": "0"
          }
        } ]
    }
  })";

}  // namespace

class KeyringServiceMigrationsUnitTest : public testing::Test {
 public:
  PrefService* GetPrefs() { return &profile_prefs_; }
  PrefService* GetLocalState() { return &local_state_; }

 protected:
  void SetUp() override {
    testing::Test::SetUp();
    RegisterProfilePrefs(profile_prefs_.registry());
    RegisterLocalStatePrefs(local_state_.registry());
    RegisterProfilePrefsForMigration(profile_prefs_.registry());

    PasswordEncryptor::GetCreateNonceCallbackForTesting() =
        base::BindLambdaForTesting([this] {
          return std::vector<uint8_t>(kEncryptorNonceSize, next_nonce_++);
        });
    PasswordEncryptor::GetCreateSaltCallbackForTesting() =
        base::BindLambdaForTesting([this] {
          return std::vector<uint8_t>(kEncryptorSaltSize, next_salt_++);
        });
  }

  void TearDown() override {
    PasswordEncryptor::GetCreateNonceCallbackForTesting() =
        base::NullCallback();
    PasswordEncryptor::GetCreateSaltCallbackForTesting() = base::NullCallback();
    testing::Test::TearDown();
  }

  static std::optional<std::string> GetWalletMnemonic(
      const std::string& password,
      KeyringService* service) {
    base::RunLoop run_loop;
    std::optional<std::string> mnemonic;
    service->GetWalletMnemonic(
        password,
        base::BindLambdaForTesting([&](const std::optional<std::string>& v) {
          mnemonic = v;
          run_loop.Quit();
        }));
    run_loop.Run();
    return mnemonic;
  }

 private:
  uint8_t next_nonce_ = 1;
  uint8_t next_salt_ = 1;

  base::test::ScopedFeatureList feature_list_{
      features::kBraveWalletZCashFeature};

  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable profile_prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
};

class KeyringServiceMigrationsLegacyIterationsUnitTest
    : public KeyringServiceMigrationsUnitTest {
 public:
  void SetupLegacyKeyringPrefs() {
    GetPrefs()->SetDict(kBraveWalletKeyrings,
                        ParseJsonDict(kLegacyKeyringsDict143));

    brave_wallet::MigrateObsoleteProfilePrefs(GetPrefs());
  }

  void ValidatePrefs() {
    EXPECT_EQ(GetPrefs()->GetDict(kBraveWalletKeyrings),
              ParseJsonDict(kUpgradedKeyringsDict143));
    EXPECT_TRUE(GetPrefs()->GetBoolean(kBraveWalletMnemonicBackedUp));
    EXPECT_EQ(GetPrefs()->GetString(kBraveWalletEncryptorSalt),
              "AQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQE=");
    EXPECT_EQ(GetPrefs()->GetDict(kBraveWalletMnemonic), ParseJsonDict(R"(
    {
      "ciphertext": "nGGZGCqH2kCOX1lWunTKLNO7dxcdbWR6+STF/Osppkqz5LzQHDuf5KHiDjT2ib0L3v0arC/vcmdtmmMPtgHZKMhV7a4fARqzO0fyWQq0ZE7srYEsHZB3Y2q6",
      "nonce": "AQEBAQEBAQEBAQEB"
    })"));

    EXPECT_TRUE(
        GetPrefs()->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated));
  }

  void ValidateAccounts(KeyringService& service) {
    EXPECT_EQ(GetWalletMnemonic(kPassword, &service), kMnemonicDivideCruise);
    const auto& all_accounts = service.GetAllAccountInfos();
    EXPECT_EQ(all_accounts.size(), 7u);
    EXPECT_EQ(all_accounts[0]->address,
              "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db");
    EXPECT_EQ(all_accounts[1]->address,
              "0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0");
    EXPECT_EQ(all_accounts[2]->address,
              "0xDc06aE500aD5ebc5972A0D8Ada4733006E905976");
    EXPECT_EQ(all_accounts[3]->address,
              "f1qjidlytseoouzfhsgzczf3ettbhuaezorczeava");
    EXPECT_EQ(all_accounts[4]->address,
              "f1syhomjrwhjmavadwmrofjpiocb6r72h4qoy7ucq");
    EXPECT_EQ(all_accounts[5]->address,
              "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8");
    EXPECT_EQ(all_accounts[6]->address,
              "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ");
  }
};

TEST_F(KeyringServiceMigrationsLegacyIterationsUnitTest, NoMigration) {
  KeyringService service(nullptr, GetPrefs(), GetLocalState());

  base::MockCallback<KeyringService::CreateWalletCallback> callback;
  EXPECT_CALL(callback, Run(_));
  service.CreateWallet(kPassword, callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);

  EXPECT_FALSE(GetPrefs()->GetBoolean(kBraveWalletMnemonicBackedUp));
  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated));

  // Just two default accounts.
  const auto& all_accounts = service.GetAllAccountInfos();
  EXPECT_EQ(all_accounts.size(), 2u);
}

TEST_F(KeyringServiceMigrationsLegacyIterationsUnitTest, MigrateOnUnlock) {
  SetupLegacyKeyringPrefs();

  KeyringService service(nullptr, GetPrefs(), GetLocalState());

  base::MockCallback<KeyringService::UnlockCallback> callback;
  EXPECT_CALL(callback, Run(true));
  service.Unlock(kPassword, callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);

  ValidatePrefs();
  ValidateAccounts(service);
}

TEST_F(KeyringServiceMigrationsLegacyIterationsUnitTest,
       MigrateOnUnlockAfterInvalidPassword) {
  SetupLegacyKeyringPrefs();

  KeyringService service(nullptr, GetPrefs(), GetLocalState());

  base::MockCallback<KeyringService::UnlockCallback> callback;
  EXPECT_CALL(callback, Run(false));
  service.Unlock("wrong password", callback.Get());
  EXPECT_TRUE(service.IsLockedSync());
  testing::Mock::VerifyAndClearExpectations(&callback);

  EXPECT_CALL(callback, Run(true));
  service.Unlock(kPassword, callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);

  ValidatePrefs();
  ValidateAccounts(service);
}

TEST_F(KeyringServiceMigrationsLegacyIterationsUnitTest,
       MigrateOnValidatePassword) {
  SetupLegacyKeyringPrefs();

  KeyringService service(nullptr, GetPrefs(), GetLocalState());

  base::MockCallback<KeyringService::UnlockCallback> callback;
  EXPECT_CALL(callback, Run(true));
  service.ValidatePassword(kPassword, callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);

  ValidatePrefs();

  base::MockCallback<KeyringService::UnlockCallback> unlock_callback;
  EXPECT_CALL(unlock_callback, Run(true));
  service.Unlock(kPassword, unlock_callback.Get());
  testing::Mock::VerifyAndClearExpectations(&unlock_callback);

  ValidateAccounts(service);
}

TEST_F(KeyringServiceMigrationsLegacyIterationsUnitTest,
       MigrateOnValidatePasswordAfterInvalidPassword) {
  SetupLegacyKeyringPrefs();

  KeyringService service(nullptr, GetPrefs(), GetLocalState());

  base::MockCallback<KeyringService::UnlockCallback> callback;
  EXPECT_CALL(callback, Run(false));
  service.ValidatePassword("wrong password", callback.Get());
  EXPECT_TRUE(service.IsLockedSync());
  testing::Mock::VerifyAndClearExpectations(&callback);

  EXPECT_EQ(GetPrefs()->GetDict(kBraveWalletMnemonic), base::Value::Dict());

  EXPECT_CALL(callback, Run(true));
  service.ValidatePassword(kPassword, callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);

  ValidatePrefs();

  base::MockCallback<KeyringService::UnlockCallback> unlock_callback;
  EXPECT_CALL(unlock_callback, Run(true));
  service.Unlock(kPassword, unlock_callback.Get());
  testing::Mock::VerifyAndClearExpectations(&unlock_callback);

  ValidateAccounts(service);
}

TEST_F(KeyringServiceMigrationsLegacyIterationsUnitTest,
       MigrateOnRestoreWallet) {
  SetupLegacyKeyringPrefs();

  KeyringService service(nullptr, GetPrefs(), GetLocalState());

  service.RestoreWallet(kMnemonicDivideCruise, kPassword, false,
                        base::DoNothing());

  ValidatePrefs();
  ValidateAccounts(service);
}

TEST_F(KeyringServiceMigrationsLegacyIterationsUnitTest,
       MigrateOnRestoreWalletWithNewPassword) {
  SetupLegacyKeyringPrefs();

  const char new_password[] = "new password";

  KeyringService service(nullptr, GetPrefs(), GetLocalState());

  service.RestoreWallet(kMnemonicDivideCruise, new_password, false,
                        base::DoNothing());

  EXPECT_EQ(GetWalletMnemonic(new_password, &service), kMnemonicDivideCruise);
  // Just two default accounts.
  const auto& all_accounts = service.GetAllAccountInfos();
  EXPECT_EQ(all_accounts.size(), 2u);
  EXPECT_EQ(all_accounts[0]->address,
            "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db");
  EXPECT_EQ(all_accounts[1]->address,
            "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8");
}

class KeyringServiceMigrationsLegacyMnemonicFormatUnitTest
    : public KeyringServiceMigrationsUnitTest {
 public:
  void SetupLegacyKeyringPrefs() {
    GetPrefs()->SetBoolean(kBraveWalletKeyringEncryptionKeysMigrated, true);
    GetPrefs()->SetDict(kBraveWalletKeyrings,
                        ParseJsonDict(kLegacyKeyringsDict166));

    brave_wallet::MigrateObsoleteProfilePrefs(GetPrefs());
  }

  void ValidatePrefs() {
    EXPECT_EQ(GetPrefs()->GetDict(kBraveWalletKeyrings),
              ParseJsonDict(kUpgradedKeyringsDict166));
    EXPECT_TRUE(GetPrefs()->GetBoolean(kBraveWalletMnemonicBackedUp));
    EXPECT_EQ(GetPrefs()->GetString(kBraveWalletEncryptorSalt),
              "AQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQE=");
    EXPECT_EQ(GetPrefs()->GetDict(kBraveWalletMnemonic), ParseJsonDict(R"(
    {
      "ciphertext": "nGGZGCqH2kCOX1lWunTKLNO7dxcdbWR6+STF/Osppkqz5LzQHDuf5KHiDjT2ib0L3v0arC/vcmdtmmMPtgHZKMhV7a4fARqzO0fyWQq0ZE7srYEsHZB3Y2q6",
      "nonce": "AQEBAQEBAQEBAQEB"
    })"));

    EXPECT_TRUE(
        GetPrefs()->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated));
  }

  void ValidateAccounts(KeyringService& service) {
    EXPECT_EQ(GetWalletMnemonic(kPassword, &service), kMnemonicDivideCruise);
    const auto& all_accounts = service.GetAllAccountInfos();
    EXPECT_EQ(all_accounts.size(), 12u);
    EXPECT_EQ(all_accounts[0]->address,
              "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db");
    EXPECT_EQ(all_accounts[1]->address,
              "0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0");
    EXPECT_EQ(all_accounts[2]->address,
              "0xDc06aE500aD5ebc5972A0D8Ada4733006E905976");
    EXPECT_EQ(all_accounts[3]->address,
              "f1qjidlytseoouzfhsgzczf3ettbhuaezorczeava");
    EXPECT_EQ(all_accounts[4]->address,
              "f1syhomjrwhjmavadwmrofjpiocb6r72h4qoy7ucq");
    EXPECT_EQ(all_accounts[5]->address,
              "t1dca7adhz5lbvin5n3qlw67munu6xhn5fpb77nly");
    EXPECT_EQ(all_accounts[6]->address,
              "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8");
    EXPECT_EQ(all_accounts[7]->address,
              "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ");

    EXPECT_EQ(all_accounts[8]->account_id->unique_key, "0_4_0_0");
    EXPECT_EQ(all_accounts[9]->account_id->unique_key, "0_5_0_0");
    EXPECT_EQ(all_accounts[10]->account_id->unique_key, "133_6_0_0");
    EXPECT_EQ(all_accounts[11]->account_id->unique_key, "133_7_0_0");
  }
};

TEST_F(KeyringServiceMigrationsLegacyMnemonicFormatUnitTest, NoMigration) {
  KeyringService service(nullptr, GetPrefs(), GetLocalState());

  base::MockCallback<KeyringService::CreateWalletCallback> callback;
  EXPECT_CALL(callback, Run(_));
  service.CreateWallet(kPassword, callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);

  EXPECT_FALSE(GetPrefs()->GetBoolean(kBraveWalletMnemonicBackedUp));
  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated));

  // Just two default accounts.
  const auto& all_accounts = service.GetAllAccountInfos();
  EXPECT_EQ(all_accounts.size(), 2u);
}

TEST_F(KeyringServiceMigrationsLegacyMnemonicFormatUnitTest, MigrateOnUnlock) {
  SetupLegacyKeyringPrefs();

  KeyringService service(nullptr, GetPrefs(), GetLocalState());

  base::MockCallback<KeyringService::UnlockCallback> callback;
  EXPECT_CALL(callback, Run(true));
  service.Unlock(kPassword, callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);

  ValidatePrefs();
  ValidateAccounts(service);
}

TEST_F(KeyringServiceMigrationsLegacyMnemonicFormatUnitTest,
       MigrateOnUnlockAfterInvalidPassword) {
  SetupLegacyKeyringPrefs();

  KeyringService service(nullptr, GetPrefs(), GetLocalState());

  base::MockCallback<KeyringService::UnlockCallback> callback;
  EXPECT_CALL(callback, Run(false));
  service.Unlock("wrong password", callback.Get());
  EXPECT_TRUE(service.IsLockedSync());
  testing::Mock::VerifyAndClearExpectations(&callback);

  EXPECT_CALL(callback, Run(true));
  service.Unlock(kPassword, callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);

  ValidatePrefs();
  ValidateAccounts(service);
}

TEST_F(KeyringServiceMigrationsLegacyMnemonicFormatUnitTest,
       MigrateOnValidatePassword) {
  SetupLegacyKeyringPrefs();

  KeyringService service(nullptr, GetPrefs(), GetLocalState());

  base::MockCallback<KeyringService::UnlockCallback> callback;
  EXPECT_CALL(callback, Run(true));
  service.ValidatePassword(kPassword, callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);

  ValidatePrefs();

  base::MockCallback<KeyringService::UnlockCallback> unlock_callback;
  EXPECT_CALL(unlock_callback, Run(true));
  service.Unlock(kPassword, unlock_callback.Get());
  testing::Mock::VerifyAndClearExpectations(&unlock_callback);

  ValidateAccounts(service);
}

TEST_F(KeyringServiceMigrationsLegacyMnemonicFormatUnitTest,
       MigrateOnValidatePasswordAfterInvalidPassword) {
  SetupLegacyKeyringPrefs();

  KeyringService service(nullptr, GetPrefs(), GetLocalState());

  base::MockCallback<KeyringService::UnlockCallback> callback;
  EXPECT_CALL(callback, Run(false));
  service.ValidatePassword("wrong password", callback.Get());
  EXPECT_TRUE(service.IsLockedSync());
  testing::Mock::VerifyAndClearExpectations(&callback);

  EXPECT_EQ(GetPrefs()->GetDict(kBraveWalletMnemonic), base::Value::Dict());

  EXPECT_CALL(callback, Run(true));
  service.ValidatePassword(kPassword, callback.Get());
  testing::Mock::VerifyAndClearExpectations(&callback);

  ValidatePrefs();

  base::MockCallback<KeyringService::UnlockCallback> unlock_callback;
  EXPECT_CALL(unlock_callback, Run(true));
  service.Unlock(kPassword, unlock_callback.Get());
  testing::Mock::VerifyAndClearExpectations(&unlock_callback);

  ValidateAccounts(service);
}

TEST_F(KeyringServiceMigrationsLegacyMnemonicFormatUnitTest,
       MigrateOnRestoreWallet) {
  SetupLegacyKeyringPrefs();

  KeyringService service(nullptr, GetPrefs(), GetLocalState());

  service.RestoreWallet(kMnemonicDivideCruise, kPassword, false,
                        base::DoNothing());

  ValidatePrefs();
  ValidateAccounts(service);
}

TEST_F(KeyringServiceMigrationsLegacyMnemonicFormatUnitTest,
       MigrateOnRestoreWalletWithNewPassword) {
  SetupLegacyKeyringPrefs();

  const char new_password[] = "new password";

  KeyringService service(nullptr, GetPrefs(), GetLocalState());

  service.RestoreWallet(kMnemonicDivideCruise, new_password, false,
                        base::DoNothing());

  EXPECT_EQ(GetWalletMnemonic(new_password, &service), kMnemonicDivideCruise);
  // Just two default accounts.
  const auto& all_accounts = service.GetAllAccountInfos();
  EXPECT_EQ(all_accounts.size(), 2u);
  EXPECT_EQ(all_accounts[0]->address,
            "0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db");
  EXPECT_EQ(all_accounts[1]->address,
            "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8");
}

}  // namespace brave_wallet
