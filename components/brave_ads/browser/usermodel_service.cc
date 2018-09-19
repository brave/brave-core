#include "usermodel_service.h"
#include "base/task_runner_util.h"
#include "base/bind.h"
#include "base/guid.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/i18n/time_formatting.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_runner_util.h"
#include "base/task/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "user_model.h"

#include "usermodel_callback_handler.h"

std::string LoadModelTaskRunner(
    const base::FilePath& path) {
  std::string data;
  bool success = base::ReadFileToString(path, &data);

  // Make sure the file isn't empty.
  if (!success || data.empty()) {
    LOG(ERROR) << "Failed to read file: " << path.MaybeAsASCII();
    return std::string();
  }
  return data;
}

void brave_ads::UsermodelService::OnModelLoaded(const std::string& data) {
  usermodel_.initializePageClassifier(data);
}

brave_ads::UsermodelService::UsermodelService(Profile* profile) :
    file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
        {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::BLOCK_SHUTDOWN})) {

    base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::Bind(&LoadModelTaskRunner, profile->GetPath().AppendASCII("taxonomy_model.json")),
      base::Bind(&UsermodelService::OnModelLoaded, AsWeakPtr()));
}

brave_ads::UsermodelService::~UsermodelService() {
  //file_task_runner_->DeleteSoon(FROM_HERE, publisher_info_backend_.release());
}
