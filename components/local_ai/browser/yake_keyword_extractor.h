/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCAL_AI_BROWSER_YAKE_KEYWORD_EXTRACTOR_H_
#define BRAVE_COMPONENTS_LOCAL_AI_BROWSER_YAKE_KEYWORD_EXTRACTOR_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"

namespace local_ai {

struct KeywordScore {
  std::string keyword;
  double score;
};

// Co-occurrence graph edge structure
struct GraphEdge {
  int target_id;
  double tf = 0.0;  // Co-occurrence frequency
};

// Co-occurrence graph implementation (NetworkX-like functionality)
class CooccurrenceGraph {
 public:
  CooccurrenceGraph();
  ~CooccurrenceGraph();

  void AddNode(int node_id);
  void AddEdge(int from_id, int to_id, double weight = 0.0);
  void IncrementEdgeWeight(int from_id, int to_id, double increment = 1.0);

  std::vector<GraphEdge> GetOutEdges(int node_id) const;
  std::vector<GraphEdge> GetInEdges(int node_id) const;
  bool HasEdge(int from_id, int to_id) const;
  double GetEdgeWeight(int from_id, int to_id) const;

 private:
  std::map<int, std::vector<GraphEdge>> out_edges_;
  std::map<int, std::vector<GraphEdge>> in_edges_;
  std::set<int> nodes_;
};

// Single word term representation (following reference SingleWord class)
class SingleWord {
 public:
  SingleWord(const std::string& unique_term, int id, CooccurrenceGraph* graph);
  ~SingleWord();

  // Core properties
  int GetId() const { return id_; }
  const std::string& GetUniqueTerm() const { return unique_term_; }
  bool IsStopword() const { return stopword_; }
  void SetStopword(bool value) { stopword_ = value; }

  // Statistics
  double GetTf() const { return tf_; }
  void SetTf(double value) { tf_ = value; }
  double GetTfA() const { return tf_a_; }
  void SetTfA(double value) { tf_a_ = value; }
  double GetTfN() const { return tf_n_; }
  void SetTfN(double value) { tf_n_ = value; }
  double GetH() const { return h_; }
  void SetH(double value) { h_ = value; }

  // Features
  double GetWfreq() const { return wfreq_; }
  double GetWcase() const { return wcase_; }
  double GetWrel() const { return wrel_; }
  double GetWpos() const { return wpos_; }
  double GetWspread() const { return wspread_; }

  // Occurrence tracking
  void AddOccur(const std::string& tag,
                int sent_id,
                int pos_sent,
                int pos_text);
  const std::map<int, std::vector<std::pair<int, int>>>& GetOccurs() const {
    return occurs_;
  }

  // Feature computation
  struct GraphMetrics {
    int wdr = 0;       // Word different right (outgoing edges count)
    double wir = 0.0;  // Word importance right (outgoing edge weights sum)
    double pwr = 0.0;  // Probability weight right (wdr/wir)
    int wdl = 0;       // Word different left (incoming edges count)
    double wil = 0.0;  // Word importance left (incoming edge weights sum)
    double pwl = 0.0;  // Probability weight left (wdl/wil)
  };

  GraphMetrics GetGraphMetrics() const;
  void UpdateH(const struct DocumentStats& stats);

 private:
  int id_;
  std::string unique_term_;
  raw_ptr<CooccurrenceGraph> graph_;
  bool stopword_ = false;

  // Statistics
  double tf_ = 0.0;
  double tf_a_ = 0.0;  // Acronym frequency
  double tf_n_ = 0.0;  // Proper noun frequency
  double h_ = 0.0;     // Final score

  // Features
  double wfreq_ = 0.0;
  double wcase_ = 0.0;
  double wrel_ = 1.0;
  double wpos_ = 1.0;
  double wspread_ = 0.0;

  // Occurrence data: sentence_id -> [(pos_sent, pos_text), ...]
  std::map<int, std::vector<std::pair<int, int>>> occurs_;
};

// Composed word (n-gram) representation
class ComposedWord {
 public:
  ComposedWord(const std::vector<SingleWord*>& terms,
               const std::string& keyword);
  ~ComposedWord();

  // Properties
  const std::string& GetKeyword() const { return kw_; }
  const std::string& GetUniqueKeyword() const { return unique_kw_; }
  size_t GetSize() const { return size_; }
  double GetTf() const { return tf_; }
  void SetTf(double value) { tf_ = value; }
  double GetH() const { return h_; }
  void SetH(double value) { h_ = value; }

  // Validation
  bool IsValid() const;
  bool StartsOrEndsWithStopword() const { return start_or_end_stopwords_; }

  // Feature computation
  void UpdateH();

  const std::vector<SingleWord*>& GetTerms() const { return terms_; }

 private:
  std::vector<SingleWord*> terms_;
  std::string kw_;         // Original keyword
  std::string unique_kw_;  // Lowercase keyword
  size_t size_;
  double tf_ = 0.0;
  double h_ = 1.0;
  bool start_or_end_stopwords_ = false;
  std::set<std::string> tags_;
};

// Document statistics structure
struct DocumentStats {
  int number_of_sentences = 0;
  int number_of_words = 0;
  double max_tf = 0.0;
  double avg_tf = 0.0;
  double std_tf = 0.0;
};

class YakeKeywordExtractor {
 public:
  YakeKeywordExtractor();
  ~YakeKeywordExtractor();

  std::vector<KeywordScore> ExtractKeywords(const std::string& text,
                                            size_t max_keywords = 10,
                                            size_t max_ngram_size = 3);

  // New version with window_size parameter
  std::vector<KeywordScore> ExtractKeywords(const std::string& text,
                                            size_t max_keywords,
                                            size_t max_ngram_size,
                                            size_t window_size);

 private:
  // Core data structures following reference YAKE
  std::unique_ptr<CooccurrenceGraph> graph_;
  std::map<std::string, std::unique_ptr<SingleWord>> terms_;
  std::map<std::string, std::unique_ptr<ComposedWord>> candidates_;
  std::vector<std::vector<std::string>> sentences_str_;
  int next_term_id_;

  // Text processing pipeline following reference implementation
  void BuildDataStructures(const std::string& text,
                           size_t window_size,
                           size_t n);
  void ProcessSentence(const std::vector<std::string>& sentence,
                       int sentence_id,
                       int& pos_text,
                       size_t window_size,
                       size_t n);
  void ProcessWord(
      const std::string& word,
      const std::string& tag,
      int sentence_id,
      int pos_sent,
      int& pos_text,
      std::vector<std::pair<std::string, SingleWord*>>& block_of_words,
      size_t window_size,
      size_t n);

  // Graph and feature computation
  void UpdateCooccurrence(
      const std::vector<std::pair<std::string, SingleWord*>>& block_of_words,
      SingleWord* current_term,
      size_t window_size);
  void GenerateCandidates(
      const std::string& word,
      const std::string& tag,
      SingleWord* term_obj,
      const std::vector<std::pair<std::string, SingleWord*>>& block_of_words,
      size_t n);
  void BuildSingleTermFeatures();
  void BuildMultiTermFeatures();
  DocumentStats CalculateDocumentStats() const;

  // Term management
  SingleWord* GetOrCreateTerm(const std::string& unique_term);
  void AddOrUpdateComposedWord(std::unique_ptr<ComposedWord> cand);

  // Text processing utilities following reference implementation
  std::vector<std::vector<std::string>> TokenizeSentences(
      const std::string& text);
  std::string PreFilter(const std::string& text);
  std::string GetTag(const std::string& word,
                     int position,
                     const std::set<char>& exclude);

  // Deduplication following reference YAKE
  std::vector<KeywordScore> DeduplicateKeywords(
      const std::vector<std::pair<std::string, double>>& candidates,
      size_t max_keywords,
      double dedup_threshold = 0.9);
  double CalculateStringSimilarity(const std::string& str1,
                                   const std::string& str2);

  // Utility functions
  bool IsStopWord(const std::string& word);
  std::string ToLowerCase(const std::string& text);
};

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_BROWSER_YAKE_KEYWORD_EXTRACTOR_H_
