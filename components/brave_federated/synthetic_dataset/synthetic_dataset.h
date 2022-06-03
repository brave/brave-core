#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_SYNTHETIC_DATASET_SYNTHETIC_DATASET_H
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_SYNTHETIC_DATASET_SYNTHETIC_DATASET_H

#include <vector>

namespace brave_federated {

class SyntheticDataset {
 public:
  // Generates the synthetic dataset of size size around given vector m of size
  // ms_size and given bias b.
  SyntheticDataset(std::vector<float> ms, float b, size_t size);
  SyntheticDataset(float alpha, float beta, int num_features, size_t size);
  SyntheticDataset(std::vector<std::vector<float>> data_points);
  SyntheticDataset(std::vector<std::vector<float>> W,
                                     std::vector<float> b,
                                     int num_features,
                                     size_t size);

  ~SyntheticDataset();

  // Returns the size of the dataset.
  size_t size();

  // Returns the dataset.
  std::vector<std::vector<float>> DataPoints();

  SyntheticDataset SeparateTestData(int num_training);

  int get_features_count();

  void DumpToCSV(std::string prefix);

 private:
  std::vector<std::vector<float>> data_points_;
  
  float Softmax(float z);
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_SYNTHETIC_DATASET_SYNTHETIC_DATASET_H
