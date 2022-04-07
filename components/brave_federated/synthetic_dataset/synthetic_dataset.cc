#include "brave/components/brave_federated/synthetic_dataset/synthetic_dataset.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>

namespace brave_federated {

SyntheticDataset::SyntheticDataset(std::vector<float> ms, float b, size_t size) {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<> distr(-10.0, 10.0);
  std::cout << "True parameters: " << std::endl;
  for (int i = 0; i < (int)ms.size(); i++) {
    std::cout << std::fixed << "  m" << i << " = " << ms[i] << std::endl;
  }

  std::cout << "  b = " << std::fixed << b << std::endl;

  std::vector<std::vector<float>> xs(size, std::vector<float>(ms.size()));
  std::vector<float> ys(size, 0);
  for (int m_ind = 0; m_ind < (int)ms.size(); m_ind++) {
    std::uniform_real_distribution<float> distx(-10.0, 10.0);

    for (int i = 0; i < (int)size; i++) {
      xs[i][m_ind] = distx(mt);
    }
  }

  for (int i = 0; i < (int)size; i++) {
    ys[i] = b;
    for (int m_ind = 0; m_ind < (int)ms.size(); m_ind++) {
      ys[i] += ms[m_ind] * xs[i][m_ind];
    }
  }

  std::vector<std::vector<float>> data_points;
  for (int i = 0; i < (int)size; i++) {
    std::vector<float> data_point;
    data_point.insert(data_point.end(), xs[i].begin(), xs[i].end());
    data_point.push_back(ys[i]);

    data_points.push_back(data_point);
  }

  this->data_points_ = data_points;
}

SyntheticDataset::~SyntheticDataset() {}

size_t SyntheticDataset::size() {
  return this->data_points_.size();
}

int SyntheticDataset::get_features_count() {
  return this->data_points_[0].size() - 1;
}

std::vector<std::vector<float>> SyntheticDataset::DataPoints() {
  return this->data_points_;
}

}  // namespace brave_federated