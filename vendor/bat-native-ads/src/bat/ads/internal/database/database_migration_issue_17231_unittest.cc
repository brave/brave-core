/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cstdint>

#include "bat/ads/internal/container_util.h"
#include "bat/ads/internal/conversions/conversion_info.h"
#include "bat/ads/internal/conversions/conversion_queue_item_info.h"
#include "bat/ads/internal/database/tables/conversion_queue_database_table.h"
#include "bat/ads/internal/database/tables/conversions_database_table.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsDatabaseMigrationIssue17231Test : public UnitTestBase {
 protected:
  BatAdsDatabaseMigrationIssue17231Test() = default;

  ~BatAdsDatabaseMigrationIssue17231Test() override = default;

  void SetUp() override {
    ASSERT_TRUE(CopyFileFromTestPathToTempDir("database_issue_17231.sqlite",
                                              "database.sqlite"));

    UnitTestBase::SetUpForTesting(/* integration_test */ false);
  }
};

TEST_F(BatAdsDatabaseMigrationIssue17231Test, ConversionQueueDatabase) {
  // Arrange
  database::table::ConversionQueue database_table;

  // Act
  database_table.GetAll(
      [](const bool success,
         const ConversionQueueItemList& conversion_queue_items) {
        ASSERT_TRUE(success);

        ConversionQueueItemInfo conversion_queue_item;
        conversion_queue_item.campaign_id =
            "6ee347d9-acec-4a80-b108-e9335a5cbd39";
        conversion_queue_item.creative_set_id =
            "37489815-8786-4ef8-83aa-4b0737f44375";
        conversion_queue_item.creative_instance_id =
            "6c9b4c69-1ed8-4dc3-b44d-5b37a479795e";
        conversion_queue_item.advertiser_id =
            "80ec0ddb-8dbb-4009-8192-1528faa411ae";
        conversion_queue_item.conversion_id =
            "425fb519-f6c0-407f-98f6-cfff8f2b1ec7";
        conversion_queue_item.timestamp = base::Time::FromDoubleT(1627581449);

        ConversionQueueItemList expected_conversion_queue_items;
        expected_conversion_queue_items.push_back(conversion_queue_item);

        EXPECT_EQ(expected_conversion_queue_items, conversion_queue_items);
      });

  // Assert
}

TEST_F(BatAdsDatabaseMigrationIssue17231Test, ConversionsDatabase) {
  // Arrange
  database::table::Conversions database_table;

  AdvanceClock(TimeFromDateString("28 July 2021"));

  // Act
  database_table.GetAll([](const bool success,
                           const ConversionList& conversions) {
    ASSERT_TRUE(success);

    const std::vector<std::string> creative_set_ids = {
        "88ef5a12-6436-4be6-955d-b181407d58c5",
        "f797c614-b55e-41dd-8bcc-46e4c450f6a2",
        "7f35ca64-e162-4912-9641-649bdd6c0e5e",
        "a82d4920-a487-4a9e-bd9b-1bf27b1db5b5",
        "4a67b1ea-8822-4a1f-97d9-27ae99a863fd",
        "33a26f73-e778-4ccb-8eb8-a05c88b35d58",
        "1d07b56b-c019-4402-aeb1-f823fe68cb34",
        "94f81f29-2b2d-40c3-bd1d-2c2bccf3a528",
        "bfd4de55-92c1-4a29-98bb-e9462597d3de",
        "5007ed46-e961-4f31-9e53-c66b20fab25b",
        "131fd2bd-90d6-4212-9ed6-53672df8b5b2",
        "c99fadb5-cca7-459f-abc1-980d588e4bc0",
        "1988e8a6-a126-40b7-87a6-64c699a96d03",
        "5ffa1ff8-685a-4142-ae89-309c6de3c4c4",
        "aba0fc68-a5ca-4714-9f14-7476318993d0",
        "60221644-a266-4bfb-95c3-08cd7f963919",
        "6a70daa0-1601-43ce-b052-824cb466d1a9",
        "2aba8ede-0690-4418-91e4-ad9af52138da",
        "1866bc8f-672d-42c9-8106-38d0eb37c5de",
        "37bd5270-61d0-4ac2-8a70-c13d666f4c72",
        "b4d40f03-738f-4926-8b4d-8503ccce07c9",
        "94fdda8d-14b8-41e0-9256-3eed1942c47f",
        "fc041ff0-ffb7-4fbd-a22b-0c1ed974a342",
        "fcad3d3c-4560-43df-8b7e-018194e2df00",
        "86bbfcbe-643a-41bf-898e-28117c131d8f",
        "a390a27a-6d25-448a-8d3e-c9997e47bce8",
        "0de537eb-f5ae-4a80-a73d-35973e28ac14",
        "9e1d0de0-c788-4d23-9543-29cee7d74655",
        "fe6ecec2-43df-4f82-a9b8-adcaba2c66d7",
        "f2985b68-ea65-49aa-a3c8-80f1b09ec004",
        "e41c7e4d-e30a-43fc-859d-9050d987052b",
        "3b8a7538-7ce1-475e-a4ce-43d3ffa04f1f",
        "435e020a-5c16-4ed1-9dfa-cc172c54f34a",
        "6de2c92c-9afe-4542-96f9-42482339b0fa",
        "f2d8f5df-85b1-45be-9689-2e88e8d77972",
        "47eb134f-fbee-4485-a1a3-54cfb34e578d",
        "50b658f0-11f6-4061-a3b7-2c8c91a2370f",
        "4e30e5b4-bd22-4277-891b-9f4a00ffa953",
        "cd34c3d7-2a1a-4557-b9e5-280ed6c86d1c",
        "df28cfca-66b5-4786-bed7-f2658586f81d",
        "3103654b-754a-439a-a7cc-e26509c53065",
        "61a6f59d-0cd5-4347-b0ed-74738fd9052b",
        "5b9694fc-ebb1-4087-a83a-201f0932b7fe",
        "40ba177a-9ada-4b57-9f1d-6729d5d20fb0",
        "d8ea218f-a2e5-452f-b0c5-b39b6429ac90",
        "1fa07d97-d3ed-44a2-87d4-faaf6489a85d",
        "58a30ac0-25ba-4882-899c-cad15050b5d5",
        "d96e57fb-c1f8-4383-bad1-50fe84fb1f2f",
        "e2e24a14-c285-4a76-a300-aded2dfcbeff",
        "c23c5a6a-aec6-4e50-adc0-62241a6465d1",
        "6af9a67e-6ccb-404b-9a32-992b9549e5ee",
        "d0fb1a97-b6a0-496b-a6ca-3172bf55ebdb",
        "425c2963-9888-4b39-915c-ed8acdabbfcc",
        "d63786ff-121e-4a0c-92aa-77518cf60d69",
        "086c5040-f7f7-4eed-a826-f8ee049e86c3",
        "133500f5-5baa-484c-af63-a7c02beb77cd",
        "724b6e14-9530-4fb9-b388-2a7f588a9062",
        "dafc82bd-42e2-4487-a816-cecdd932eabe",
        "a7dee316-640b-4c20-afa1-73f8e7ccc6ba",
        "8843525b-1785-4c23-aa99-cd81745075eb",
        "d594cb5f-c07e-4cd2-9cfe-2b26ae6cf718",
        "fa9e9a5d-cb39-41ed-8494-04725649e46a",
        "53dd50a2-1270-4a7b-90cf-3a5864aebe29",
        "9c290185-b312-4aa0-bed8-ad5171d2094b",
        "ea57cce4-3e76-4b88-af8e-708d44f0aae1",
        "bac89817-2c76-48c7-98b1-fcb6e11d60d4",
        "33a26f73-e778-4ccb-8eb8-a05c88b35d58",
        "3e6d07f9-b2e5-443b-b369-e4b7c310a94e",
        "33f1cc6f-2958-4b9d-8ed4-b7b628fd03e1",
        "c14d9bad-b79c-4aa2-9428-802082610eea",
        "13ca9f95-5a59-462d-ae4e-0ef130012484",
        "2ad19391-78b9-4542-a7df-b4e6163b5459",
        "250815d7-1fe3-4d50-8b61-5aefb8a0f6ae",
        "f702874f-91a8-480b-8eed-ccab9fa61544",
        "44c299bc-c8ab-494d-b4b9-1c9a2a091f9e",
        "ecbba03c-678b-484a-9623-ac370b9d525f",
        "39aea58a-99be-4f31-bb58-eb0347832c4d",
        "ae62c654-1397-40c9-884f-7bbe43f771a8",
        "ad9015d5-21c2-43c9-bca9-cb6fa26863bd",
        "8c8cc970-02c1-4e79-bb7a-a4fb7751f388",
        "afb9e268-4221-406e-b800-3bf1bed8aeb3",
        "112a6574-23ee-46bb-96f6-5552c160f582",
        "bf91d9d8-39a8-4549-8da8-03f7bbcd7961",
        "7e189d1b-3a43-4b3c-91d3-d11a036db650",
        "3d7d30f4-02e1-499b-ba78-3f58c5d838ac",
        "6c8b6ade-1369-4388-aff0-31125f3f96f0",
        "34203318-8cbc-4ae5-a2fb-b2a68575c090",
        "63078675-9481-45e4-b79c-a21fe9377a8a",
        "5d1e3481-bab9-4a1f-b792-5ef6dacb18e5",
        "832b1ca0-b307-488f-b400-a1f92201b8dc",
        "2c1e24b5-6761-4aaf-acc2-09764e8cdf62",
        "5fbda553-89f1-4922-82ac-8ae0819c6d93",
        "d704fcd0-200e-4f6f-bc20-2f756f749b5d",
        "9954aee1-21bf-468d-9933-c6281fb6d8e7",
        "9524e1ce-5c09-47f3-b6b9-b0253fdfdedc",
        "4214b1b3-ef4d-459c-a266-e30d02191b3b",
        "2790326c-6b91-4de9-8807-bd54a353a866",
        "6a64015e-4e87-4b22-b5cd-e82bdfc52b31",
        "1256a212-f05c-4bcb-a1b0-8d9c82455bea",
        "b5c2fc90-dc19-40a7-b322-f641dd3ba1c6",
        "47c2a7e4-1e4b-4f69-9a13-643d70d58e35",
        "120a90e4-9273-415f-93d3-3779bc5e4c05",
        "17d253ea-bfbe-4f22-a8ef-4d6fd7ea0ff8",
        "0a87fd3d-39be-4897-b1c8-a64fad34e352",
        "2b383a80-f76e-4b57-9306-d8462b6fb162",
        "dae2f14f-f6ce-4aff-a1dc-4d74f93e8511",
        "69560eb6-5049-44f3-9147-fb6e86858133",
        "ad9aa783-eb7f-46ef-bf8d-06215d40350c",
        "a8311adf-1185-4f3d-a475-2b3dd419e3fe",
        "06ee7c11-636e-4858-93c8-3d83e85f45b4",
        "ef80f3c7-d17f-4d4d-a9e1-63da2fcac105",
        "82db6fc6-18be-4f7d-af13-7f3e6fb00fb5",
        "cb509fa1-d41a-44fb-8564-3359b0b57c8b",
        "5d08915f-0795-41b0-8d77-8bae6a9616c2",
        "7734064a-aa5d-41ea-8c23-13eeb3b0e2f4",
        "578474da-329f-46e9-a62b-57a330642a72",
        "edf75535-2a76-4412-a5f3-82178ca4c158",
        "492689f1-5040-40cc-ac88-4a3e8bfe84e6",
        "966272ef-37bf-4422-8c5c-f544dc73960a",
        "f5c73e31-91fe-46ec-a304-89c93455352c",
        "f99c4182-818a-4398-986d-418856912433",
        "05106c5c-b23f-4896-831d-531a35dcdcfe",
        "4eb71245-d138-4b77-998b-8f185d89dd90",
        "62df21f5-1677-4d33-baf6-9bcc3f94f3db",
        "c7de6804-5537-4b97-9723-76a2181b42aa",
        "38808501-5f85-456e-8dd5-8fe48d598fb2",
        "38bdca09-c2b5-46c7-a7d7-e5f88d0b9f90",
        "68e8b572-183c-4184-882a-cf58764603e0",
        "89b3471b-d132-4786-9e08-02d14ba0d71c",
        "8f82f94d-45aa-40f4-be80-7cc73d85ffd0",
        "00627236-54f7-4bb1-a1a5-5cc6a6355381",
        "a82d4920-a487-4a9e-bd9b-1bf27b1db5b5",
        "bcee59ce-0a4c-4171-9f60-e21c0ba42616",
        "92fb0bbc-c6b5-4360-8e85-23103f9df9ce",
        "fff43e8f-c8a8-4a3d-89fe-b287ea9b62d0",
        "7b4c9832-bbb6-49df-bf6d-136f1dbf9db2",
        "17f19fec-050c-4c06-8586-c3640c92827a",
        "e4a1a0f1-44f9-4d7c-8f16-b67ce9551630",
        "5cb8cb2b-f950-4a97-8dea-2372a0ba658c",
        "b0ce8c8c-dfb6-474c-be98-c8fef2f2550b",
        "9e02af89-0046-4d0c-89d2-0250de721a68",
        "6b0fbc8c-e5eb-4cb7-9baa-7d4850221d3e",
        "e591a5fc-ee10-485e-93ce-9aa58342655b",
        "0116e4d7-e826-4c18-9a1a-5ca0316fcfed",
        "7f1a267a-7b5e-4956-8ff6-4832ede3fe79",
        "15953d1e-624d-4e9f-942b-93bbfae37096",
        "72b2b634-0de9-4f0f-811c-f9031ed03f2a",
        "ac6b0a87-8862-470f-95b2-52473796cc0b",
        "5f1ede27-c3e6-46de-bbbb-1da69c95f3f0",
        "442e701d-c184-4b18-be76-b305a116a3f2",
        "cb189050-d81d-44c1-8a99-0e78313302f3",
        "72a74d3c-b2c8-4233-9df5-94820206b0de",
        "6a63eba3-d4d6-469d-b5a8-6a58c5b3b201",
        "47c94060-f041-4655-a63b-13b98a4b09de",
        "9193c3d3-a4a1-468d-b6a6-e2aaaab94c7f",
        "811de1d6-415c-4c65-ae85-045608aa2edc",
        "169b095d-fff8-41a7-8486-2396ba67d98f",
        "34beae2b-bd84-4521-9122-478b7f6ce88d",
        "d52321bc-52d3-419f-babb-1c94158beb93",
        "cdb2dd80-682f-481a-a32d-f973abe01c58",
        "f0352cb3-cc28-4c7a-a8b7-ecfa35f5843d",
        "b12c9688-0d15-4c26-b812-f8301bc67f30",
        "e8dd7ced-c3a7-43de-b366-2a6a0eb80b40",
        "1c45dff5-16c1-4204-80f2-1e8bcce79050",
        "762fc8d9-9c46-4657-ad8e-77f6eb2500a0",
        "532c941d-2a2d-4954-80ac-8519283d7b44",
        "925660dc-e5d7-4c43-abe9-c8464a051244",
        "12a6a5d5-cf5e-437b-b9b3-37331d474a90",
        "8519a399-c723-4b0d-b691-3624be61e444",
        "94af1beb-7b56-4f0f-bfd2-945b7f4678a6",
        "806c4711-0403-4c24-9f86-63dfc3788580",
        "0bd562ca-31ac-4fda-9aaa-3e3ab758fb09",
        "b82ba0ea-6a67-4925-bfc5-bebf1f1711fc",
        "c8f4d375-4fe0-4bcd-aa03-e192ea3596f9",
        "e3fa4776-d529-4ae9-9c6e-7741232ee1df",
        "755e6092-854b-4722-9c34-fa5bdd62565d",
        "1bb7a0d7-f45a-4ec6-ae2c-f8cdae411452",
        "ae6f6206-06f1-4a51-9b19-c4464ffbec32",
        "cfdda5b9-b9bd-4d93-b20a-7f34e944f860",
        "17c29da6-1198-4e98-b980-ccf1548871e4",
        "27f5721e-71f3-4dd5-8250-3fe65f3b97bb",
        "e8bfe05c-2a35-4963-8b05-c840403f5eab",
        "02785f38-31cf-41e0-a255-8b5040618611",
        "51a59094-61bb-44da-a1b5-da5939f82d13",
        "596d782d-ad3b-40c9-9fac-bb8d4dfc7dd0",
        "c8fa03b7-de25-4bfc-ba02-a40ba790c897",
        "b2330a7b-e302-4b6d-863d-dd40dacce411",
        "d440792f-c93d-4790-846a-3522635f00d2",
        "682b5e9a-0dfd-4e88-a013-74e7e967ddab"};

    const std::vector<std::string> types = {
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postclick",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postclick",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview", "postview", "postview", "postview",
        "postview", "postview", "postview"};

    const std::vector<std::string> url_patterns = {
        "https://saltlending.com/waitlist-thankyou/*",
        "https://ambercrypto.com/",
        "https://ambercrypto.com/",
        "https://www.undergroundcellar.com/wine-deals/rare-red-ft-harlan/"
        "checkout/thanks/*",
        "https://www.redbubble.com/check-out/thanks?*",
        "https://kusama.network/thankyou/",
        "https://emiswap.com/#/pool",
        "https://play.upland.me/verify?token=*",
        "https://ambercrypto.com/",
        "https://ambercrypto.com/",
        "https://ambercrypto.com/",
        "https://ambercrypto.com/",
        "https://ambercrypto.com/",
        "https://ambercrypto.com/",
        "https://www.coinbase.com/dashboard*",
        "https://www.coinbase.com/dashboard*",
        "https://www.coinbase.com/dashboard*",
        "https://www.coinbase.com/dashboard*",
        "https://www.coinbase.com/dashboard*",
        "https://www.coinbase.com/dashboard*",
        "https://www.coinbase.com/dashboard*",
        "https://www.coinbase.com/dashboard*",
        "https://www.coinbase.com/dashboard*",
        "https://www.coinbase.com/dashboard*",
        "https://www.coinbase.com/dashboard*",
        "https://www.coinbase.com/dashboard*",
        "https://www.coinbase.com/dashboard*",
        "https://www.coinbase.com/dashboard*",
        "https://www.coinbase.com/dashboard*",
        "https://www.coinbase.com/dashboard*",
        "https://www.coinbase.com/dashboard*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://portal.confidently.com/on-boarding/user-details",
        "https://tezos.com/discover/sign-up-confirmation",
        "https://app.bitcoinira.com/application?e=appsubmitted*",
        "https://tezos.com/hello-world/sign-up-confirmation",
        "https://choice-app.kingdomtrust.com/Account/RegisterConfirmSent",
        "https://www.atlassian.com/ondemand/signup/"
        "checkemail?accountName=*&products=jira-servicedesk.ondemand",
        "https://buy.norton.com/estore/checkOutConfirmation*",
        "https://www.bybit.com/",
        "https://frontofficesports.com/thankyousubmission/*",
        "https://sps.northwestern.edu/info/msis-thankyou.php*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://www.coinpayments.net/register-activate-*",
        "https://kusama.network/thankyou/*",
        "https://play.upland.me/verify?token=*",
        "https://om.elvenar.com/ox/us/?ref=brave_us_us",
        "https://om.elvenar.com/ox/us/?ref=brave_us_us",
        "https://om.elvenar.com/ox/us/?ref=brave_us_us",
        "https://platform.nexo.io/borrow-complete",
        "https://platform.nexo.io/borrow-complete",
        "https://www.masterclass.com/onboarding",
        "https://www.masterclass.com/onboarding",
        "https://www.masterclass.com/onboarding",
        "https://www.masterclass.com/onboarding",
        "https://www.masterclass.com/onboarding",
        "https://www.masterclass.com/onboarding",
        "https://portal.axion.network/"
        "?utm_source=brave&utm_medium=pushnotification&utm_campaign="
        "BraveConversion",
        "https://crypto.coinsmart.com/?registered",
        "https://secure2.sophos.com/*/thank-you.aspx*",
        "https://unstoppabledomains.com/thank-you",
        "https://unstoppabledomains.com/thank-you",
        "https://unstoppabledomains.com/thank-you",
        "https://ambercrypto.com/",
        "https://store.bitdefender.com/order/finish.php*",
        "https://www.joinhoney.com/welcome*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.bitcoinira.com/application?e=appsubmitted*",
        "https://bittrustira.com/congratulations/",
        "https://play.upland.me/verify?token=*",
        "https://lustre.ai/extension/welcome/",
        "https://platform.nexo.io/borrow-complete",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://smartasset.com/retirement/financial-advisor*",
        "https://exchange.gemini.com/register/confirm-phone",
        "https://compassmining.io/dashboard/hosting-orders?checkoutId=*",
        "https://www.coinbase.com/dashboard*",
        "https://www.coinbase.com/dashboard*",
        "https://autoinsurance*.progressivedirect.com/*/UQA/Quote/"
        "RatePackageDetails",
        "https://autoinsurance*.progressivedirect.com/*/UQA/Quote/"
        "RatePackageDetails",
        "https://app.stash.com/sign-up/introduction*",
        "https://newsletter.thedefiant.io*",
        "https://www.fool.com/order/thank-you/",
        "https://brave.tapnetwork.io/",
        "https://dreamsitegurus.com/order-confirmation",
        "https://portal.confidently.com/on-boarding/user-details",
        "https://buy.norton.com/estore/checkOutConfirmation*",
        "https://www.undergroundcellar.com/wine-deals/rare-red-wines/checkout/"
        "thanks/*",
        "https://womplay.io/register/ready*",
        "https://try.codility.com/thank-you/*",
        "https://live.sovryn.app*",
        "https://app.youhodler.com/sign-up*",
        "https://about.sourcegraph.com/get-started*",
        "https://funnelhackingsecrets.com/funnel-stacking-ty*",
        "https://marketplace.ovr.ai/profile*",
        "https://saddle.exchange/",
        "https://dashboard.keep.network/delegations/granted*",
        "https://play.upland.me/verify?token=*",
        "https://app.youhodler.com/sign-up*",
        "https://stake.axion.network/",
        "https://play.upland.me/verify?token=*",
        "https://platform.nexo.io/borrow-complete",
        "https://platform.nexo.io/borrow-complete",
        "https://www.realvision.com/checkout/crypto",
        "https://portal.smartfi.com/account-verification*",
        "https://app.firstbase.io/payment/done*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://app.blockfi.com/signup/*",
        "https://www.delta.exchange/offers/?_target=brave*",
        "https://stake.axion.network/",
        "https://unstoppabledomains.com/thank-you",
        "https://om.elvenar.com/ox/us/?ref=brave_us_us",
        "https://om.elvenar.com/ox/us/?ref=brave_us_us",
        "https://om.elvenar.com/ox/us/?ref=brave_us_us",
        "https://om.elvenar.com/ox/us/?ref=brave_us_us",
        "https://om.elvenar.com/ox/us/?ref=brave_us_us",
        "https://om.elvenar.com/ox/us/?ref=brave_us_us",
        "https://om.elvenar.com/ox/us/?ref=brave_us_us",
        "https://om.elvenar.com/ox/us/?ref=brave_us_us",
        "https://om.elvenar.com/ox/us/?ref=brave_us_us",
        "https://greenrabbitgame.io/",
        "https://bitbuy.ca/en/sign-up/?c=Brave",
        "https://crypto.coinsmart.com/?registered",
        "https://web.newton.co/welcome",
        "https://web.newton.co/welcome",
        "https://brave.com/search/thank-you/",
        "https://netcoins.app/dashboard",
        "https://www.grammarly.com/"
        "signup?breadcrumbs=true&utm_source=chrome&page=install&install=true&"
        "utm_medium=navigation",
        "https://app.cakedefi.com/*",
        "https://app.cakedefi.com/",
        "https://app.cakedefi.com/*",
        "https://app.cakedefi.com/*",
        "https://app.cakedefi.com/*",
        "https://www.bybit.com/",
        "https://marsgenesis.com/?ref=brave*",
        "https://www.skiff.org/accept*"};

    const std::vector<int> observation_windows = {
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 7,  30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 7,  30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
        30, 30, 30, 30, 30, 30, 30, 30, 30};

    const std::vector<int64_t> expiry_timestamps = {
        1627631040, 1627585140, 1627585140, 1633572000, 1627668000, 1628270400,
        1627790340, 1627619580, 1627577940, 1627577940, 1627577940, 1627577940,
        1627577940, 1627577940, 1627658160, 1627658160, 1627658160, 1627658160,
        1627658160, 1627658160, 1627658160, 1627658160, 1627658160, 1627658160,
        1627658160, 1627658160, 1627658160, 1627658160, 1627658160, 1627658160,
        1627658160, 1627657680, 1627657680, 1627657680, 1627657680, 1627657680,
        1627657680, 1627657680, 1627657680, 1627657680, 1627657680, 1627657680,
        1627657680, 1629124800, 1627668000, 1627660380, 1627668000, 1628998500,
        1627612200, 1627831680, 1627657800, 1627612440, 1628461860, 1627657680,
        1627657680, 1627657680, 1627657680, 1627657680, 1627657680, 1627657680,
        1627657680, 1627657680, 1627657680, 1627657680, 1627657680, 1627657560,
        1627790340, 1627659180, 1627660620, 1627660620, 1627660620, 1627657620,
        1627657620, 1627660980, 1627660980, 1627660980, 1627660980, 1627660980,
        1627660980, 1627668000, 1630319100, 1628230800, 1630335900, 1630335900,
        1630335900, 1630337340, 1631419080, 1628402340, 1630337640, 1630337640,
        1630337640, 1630337640, 1630337640, 1630337640, 1630337640, 1630337640,
        1630337640, 1630337640, 1630337640, 1630337640, 1630337820, 1630260000,
        1630338480, 1630302780, 1630337700, 1630309620, 1630309620, 1630309620,
        1630309620, 1630309620, 1630309620, 1630309620, 1630309620, 1630309620,
        1630309620, 1630309620, 1630309620, 1630337400, 1630336560, 1631893020,
        1630339140, 1630339140, 1630306740, 1630306740, 1628953380, 1629691140,
        1635585180, 1629682560, 1628999940, 1629124800, 1638115140, 1633572000,
        1630293240, 1628827020, 1628956740, 1629691200, 1628402340, 1632196740,
        1630076940, 1633017600, 1633024800, 1630338420, 1632477600, 1631692740,
        1630339200, 1630337700, 1630337700, 1630509480, 1631419140, 1630389540,
        1630337640, 1630337640, 1630337640, 1630337640, 1630337640, 1630337640,
        1630337640, 1630337640, 1630337640, 1630337640, 1630337640, 1634202240,
        1632901560, 1630335900, 1630337280, 1630337280, 1630337280, 1630337280,
        1630337280, 1630337280, 1630337280, 1630337280, 1630337280, 1630211760,
        1631589240, 1635577860, 1631724120, 1631724120, 1630393140, 1632926520,
        1632999600, 1631635140, 1631635140, 1631635140, 1631635140, 1631635140,
        1630336740, 1633015560, 1632283140};

    ConversionList expected_conversions;

    ASSERT_EQ(189UL, conversions.size());
    for (int i = 0; i < 189; i++) {
      ConversionInfo expected_conversion;
      expected_conversion.creative_set_id = creative_set_ids.at(i);
      expected_conversion.type = types.at(i);
      expected_conversion.url_pattern = url_patterns.at(i);
      expected_conversion.observation_window = observation_windows.at(i);
      expected_conversion.expiry_timestamp = expiry_timestamps.at(i);
      expected_conversions.push_back(expected_conversion);
    }

    EXPECT_TRUE(CompareAsSets(expected_conversions, conversions));
  });

  // Assert
}

}  // namespace ads
