
#include <array>
#include <vector>

#include "base/files/file_path.h"
#include "base/sequence_checker.h"
#include "base/thread_annotations.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/prefs/pref_service.h"
#include "sql/database.h"
#include "sql/meta_table.h"

namespace brave_wallet {

struct AccountMeta {
  uint64_t account_birthday;
  uint64_t latest_scanned_block_id;
  std::string latest_scanned_block_hash;
};

// Structure describes note nullifier that was met marking some note to be spent
struct OrchardNullifier {
  // Block id where spent nullifier was met
  uint64_t block_id;
  std::array<uint8_t, 32> nullifier;
};

// Structure describes found spendable note
struct OrchardNote {
  uint64_t block_id;
  std::array<uint8_t, 32> nullifier;
  uint64_t amount;
};

class OrchardStorage {
 public:
  enum OrchardStorageErrorCode {
    kDbInitError,
    kAccountNotFound,
    kFailedToExecuteStatement,
    kInternalError
  };

  struct OrchardStorageError {
    OrchardStorageErrorCode error_code;
    std::string message;
  };

  OrchardStorage(base::FilePath path_to_database_dir);
  ~OrchardStorage();

  std::optional<OrchardStorageError> RegisterAccount(
      mojom::AccountIdPtr account_id,
      uint64_t account_birthday_block,
      const std::string& account_bithday_block_hash);
  base::expected<AccountMeta, OrchardStorageError> GetAccountMeta(
      mojom::AccountIdPtr account_id);
  std::optional<OrchardStorageError> HandleChainReorg(
      mojom::AccountIdPtr account_id,
      uint64_t reorg_block_id,
      const std::string& reorg_block_hash);
  base::expected<std::vector<OrchardNote>, OrchardStorage::OrchardStorageError>
  GetSpendableNotes(mojom::AccountIdPtr account_id);
  std::optional<OrchardStorageError> UpdateNotes(
      mojom::AccountIdPtr account_id,
      const std::vector<OrchardNote>& notes_to_add,
      const std::vector<OrchardNullifier>& notes_to_delete,
      const uint64_t latest_scanned_block,
      const std::string& latest_scanned_block_hash);
  void ResetDatabase();

 private:
  bool EnsureDbInit();
  bool CreateOrUpdateDatabase();
  bool CreateSchema();
  bool UpdateSchema();

  base::FilePath db_file_path_;
  sql::Database database_ GUARDED_BY_CONTEXT(sequence_checker_);
  sql::MetaTable meta_table_;

  raw_ptr<PrefService> prefs_;
  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace brave_wallet
