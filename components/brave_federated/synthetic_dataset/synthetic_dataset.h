#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_SYNTHETIC_DATASET_SYNTHETIC_DATASET_H
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_SYNTHETIC_DATASET_SYNTHETIC_DATASET_H

#include <vector>

namespace brave_federated {

class SyntheticDataset {
 public:
  // Generates the synthetic dataset of size size around given vector m of size
  // ms_size and given bias b.
  SyntheticDataset(std::vector<float> ms, float b, size_t size);

  ~SyntheticDataset();

  // Returns the size of the dataset.
  size_t size();

  // Returns the dataset.
  std::vector<std::vector<float>> DataPoints();

  int get_features_count();

 private:
  std::vector<std::vector<float>> data_points_;
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_SYNTHETIC_DATASET_SYNTHETIC_DATASET_H
