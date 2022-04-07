/***********************************************************************************************************
 *
 * @file typing.h
 *
 * @brief C++ Flower type definitions
 *
 * @author Lekang Jiang
 *
 * @version 1.0
 *
 * @date 03/09/2021
 *
 * ********************************************************************************************************/

#pragma once
#include <list>
#include <map>
#include <optional>
#include <string>

namespace flwr {
/**
 * This class contains C++ types corresponding to ProtoBuf types that
 * ProtoBuf considers to be "Scalar Value Types", even though some of them
 * arguably do not conform to other definitions of what a scalar is. There is no
 * "bytes" type in C++, so "string" is used instead of bytes in Python (char*
 * can also be used if needed) In C++, Class is easier to use than Union (can be
 * changed if needed) Source:
 * https://developers.google.com/protocol-buffers/docs/overview#scalar
 *
 */
class Scalar {
 public:
  // Getters
  std::optional<bool> getBool() { return b_; }
  std::optional<std::string> getBytes() { return bytes_; }
  std::optional<float> getFloat() { return f_; }
  std::optional<int> getInt() { return i_; }
  std::optional<std::string> getString() { return string_; }

  // Setters
  void setBool(bool b) { this->b_ = b; }
  void setBytes(std::string bytes) { this->bytes_ = bytes; }
  void setFloat(float f) { this->f_ = f; }
  void setInt(int i) { this->i_ = i; }
  void setString(std::string string) { this->string_ = string; }

 private:
  std::optional<bool> b_ = std::nullopt;
  std::optional<std::string> bytes_ = std::nullopt;
  std::optional<float> f_ = std::nullopt;
  std::optional<int> i_ = std::nullopt;
  std::optional<std::string> string_ = std::nullopt;
};

typedef std::map<std::string, flwr::Scalar> Metrics;

/**
 * Model parameters
 */
class Parameters {
 public:
  Parameters(){}
  Parameters(std::list<std::string> tensors, std::string tensor_type)
      : tensors_(tensors), tensor_type_(tensor_type){}

  // Getters
  std::list<std::string> getTensors() { return tensors_; }
  std::string getTensor_type() { return tensor_type_; }

  // Setters
  void setTensors(std::list<std::string> tensors) { this->tensors_ = tensors; }
  void setTensor_type(std::string tensor_type) {
    this->tensor_type_ = tensor_type;
  }

 private:
  std::list<std::string> tensors_;
  std::string tensor_type_;
};

/**
 * Response when asked to return parameters
 */
class ParametersRes {
 public:
  ParametersRes(Parameters parameters) : parameters_(parameters){}

  Parameters getParameters() { return parameters_; }
  void setParameters(Parameters p) { parameters_ = p; }

 private:
  Parameters parameters_;
};

/**
 * Fit instructions for a client
 */
class FitIns {
 public:
  FitIns(Parameters parameters, std::map<std::string, flwr::Scalar> config)
      : parameters_(parameters), config_(config){}

  // Getters
  Parameters getParameters() { return parameters_; }
  std::map<std::string, Scalar> getConfig() { return config_; }

  // Setters
  void setParameters(Parameters p) { parameters_ = p; }
  void setConfig(std::map<std::string, Scalar> config) {
    this->config_ = config;
  }

 private:
  Parameters parameters_;
  std::map<std::string, Scalar> config_;
};

/**
 * Fit response from a client
 */
class FitRes {
 public:
  FitRes(){}
  FitRes(Parameters parameters,
         int num_examples,
         int num_examples_ceil,
         float fit_duration,
         Metrics metrics)
      : parameters_(parameters),
        num_examples_(num_examples),
        fit_duration_(fit_duration),
        metrics_(metrics){}

  // Getters
  Parameters getParameters() { return parameters_; }
  int getNum_example() { return num_examples_; }
  /*std::optional<int> getNum_examples_ceil()
  {
          return num_examples_ceil;
  }*/
  std::optional<float> getFit_duration() { return fit_duration_; }
  std::optional<Metrics> getMetrics() { return metrics_; }

  // Setters
  void setParameters(Parameters p) { parameters_ = p; }
  void setNum_example(int n) { num_examples_ = n; }
  /*void setNum_examples_ceil(int n)
  {
          num_examples_ceil = n;
  }*/
  void setFit_duration(float f) { fit_duration_ = f; }
  void setMetrics(flwr::Metrics m) { metrics_ = m; }

 private:
  Parameters parameters_;
  int num_examples_;
  // std::optional<int> num_examples_ceil = std::nullopt;
  std::optional<float> fit_duration_ = std::nullopt;
  std::optional<Metrics> metrics_ = std::nullopt;
};

/**
 * Evaluate instructions for a client
 */
class EvaluateIns {
 public:
  EvaluateIns(Parameters parameters, std::map<std::string, Scalar> config)
      : parameters_(parameters), config_(config){}

  // Getters
  Parameters getParameters() { return parameters_; }
  std::map<std::string, Scalar> getConfig() { return config_; }

  // Setters
  void setParameters(Parameters p) { parameters_ = p; }
  void setConfig(std::map<std::string, Scalar> config) {
    this->config_ = config;
  }

 private:
  Parameters parameters_;
  std::map<std::string, Scalar> config_;
};

/**
 * Evaluate response from a client
 */
class EvaluateRes {
 public:
  EvaluateRes(){}
  EvaluateRes(float loss, int num_examples, float accuracy, Metrics metrics)
      : loss_(loss), num_examples_(num_examples), metrics_(metrics){}

  // Getters
  float getLoss() { return loss_; }
  int getNum_example() { return num_examples_; }
  std::optional<Metrics> getMetrics() { return metrics_; }

  // Setters
  void setLoss(float f) { loss_ = f; }
  void setNum_example(int n) { num_examples_ = n; }
  void setMetrics(Metrics m) { metrics_ = m; }

 private:
  float loss_;
  int num_examples_;
  std::optional<Metrics> metrics_ = std::nullopt;
};

typedef std::map<std::string, flwr::Scalar> Config;
typedef std::map<std::string, flwr::Scalar> Properties;

class PropertiesIns {
 public:
  PropertiesIns(){}

  std::map<std::string, flwr::Scalar> getPropertiesIns() {
    return static_cast<std::map<std::string, flwr::Scalar>>(config_);
  }

  void setPropertiesIns(Config c) { config_ = c; }

 private:
  Config config_;
};

class PropertiesRes {
 public:
  PropertiesRes(){}

  Properties getPropertiesRes() { return properties_; }

  void setPropertiesRes(Properties p) { properties_ = p; }

 private:
  Properties properties_;
};

}  // namespace flwr
