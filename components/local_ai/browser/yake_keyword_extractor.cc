/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/browser/yake_keyword_extractor.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <regex>
#include <set>
#include <sstream>
#include <unordered_set>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

namespace local_ai {

namespace {

// Stop words following reference YAKE English stopwords
const std::unordered_set<std::string> kStopWords = {
    "a",          "about",   "above",    "after",     "again",
    "against",    "all",     "am",       "an",        "and",
    "any",        "are",     "aren't",   "as",        "at",
    "be",         "because", "been",     "before",    "being",
    "below",      "between", "both",     "but",       "by",
    "can't",      "cannot",  "could",    "couldn't",  "did",
    "didn't",     "do",      "does",     "doesn't",   "doing",
    "don't",      "down",    "during",   "each",      "few",
    "for",        "from",    "further",  "had",       "hadn't",
    "has",        "hasn't",  "have",     "haven't",   "having",
    "he",         "he'd",    "he'll",    "he's",      "her",
    "here",       "here's",  "hers",     "herself",   "him",
    "himself",    "his",     "how",      "how's",     "i",
    "i'd",        "i'll",    "i'm",      "i've",      "if",
    "in",         "into",    "is",       "isn't",     "it",
    "it's",       "its",     "itself",   "let's",     "me",
    "more",       "most",    "mustn't",  "my",        "myself",
    "no",         "nor",     "not",      "of",        "off",
    "on",         "once",    "only",     "or",        "other",
    "ought",      "our",     "ours",     "ourselves", "out",
    "over",       "own",     "same",     "shan't",    "she",
    "she'd",      "she'll",  "she's",    "should",    "shouldn't",
    "so",         "some",    "such",     "than",      "that",
    "that's",     "the",     "their",    "theirs",    "them",
    "themselves", "then",    "there",    "there's",   "these",
    "they",       "they'd",  "they'll",  "they're",   "they've",
    "this",       "those",   "through",  "to",        "too",
    "under",      "until",   "up",       "very",      "was",
    "wasn't",     "we",      "we'd",     "we'll",     "we're",
    "we've",      "were",    "weren't",  "what",      "what's",
    "when",       "when's",  "where",    "where's",   "which",
    "while",      "who",     "who's",    "whom",      "why",
    "why's",      "with",    "won't",    "would",     "wouldn't",
    "you",        "you'd",   "you'll",   "you're",    "you've",
    "your",       "yours",   "yourself", "yourselves"};

const std::set<char> kExcludeChars = {'!', '"', '#', '$', '%', '&', '\'', '(',
                                      ')', '*', '+', ',', '-', '.', '/',  ':',
                                      ';', '<', '=', '>', '?', '@', '[',  '\\',
                                      ']', '^', '_', '`', '{', '|', '}',  '~'};

}  // namespace

// ====================================================================================
// CooccurrenceGraph Implementation
// ====================================================================================

CooccurrenceGraph::CooccurrenceGraph() = default;

CooccurrenceGraph::~CooccurrenceGraph() = default;

void CooccurrenceGraph::AddNode(int node_id) {
  nodes_.insert(node_id);
  if (out_edges_.find(node_id) == out_edges_.end()) {
    out_edges_[node_id] = std::vector<GraphEdge>();
  }
  if (in_edges_.find(node_id) == in_edges_.end()) {
    in_edges_[node_id] = std::vector<GraphEdge>();
  }
}

void CooccurrenceGraph::AddEdge(int from_id, int to_id, double weight) {
  AddNode(from_id);
  AddNode(to_id);

  // Check if edge already exists
  for (auto& edge : out_edges_[from_id]) {
    if (edge.target_id == to_id) {
      edge.tf = weight;
      return;
    }
  }

  // Add new edge
  out_edges_[from_id].push_back({to_id, weight});
  in_edges_[to_id].push_back({from_id, weight});
}

void CooccurrenceGraph::IncrementEdgeWeight(int from_id,
                                            int to_id,
                                            double increment) {
  // Find and increment existing edge
  for (auto& edge : out_edges_[from_id]) {
    if (edge.target_id == to_id) {
      edge.tf += increment;
      // Also update in_edges
      for (auto& in_edge : in_edges_[to_id]) {
        if (in_edge.target_id == from_id) {
          in_edge.tf += increment;
          return;
        }
      }
      return;
    }
  }

  // Create new edge if it doesn't exist
  AddEdge(from_id, to_id, increment);
}

std::vector<GraphEdge> CooccurrenceGraph::GetOutEdges(int node_id) const {
  auto it = out_edges_.find(node_id);
  return it != out_edges_.end() ? it->second : std::vector<GraphEdge>();
}

std::vector<GraphEdge> CooccurrenceGraph::GetInEdges(int node_id) const {
  auto it = in_edges_.find(node_id);
  return it != in_edges_.end() ? it->second : std::vector<GraphEdge>();
}

bool CooccurrenceGraph::HasEdge(int from_id, int to_id) const {
  auto it = out_edges_.find(from_id);
  if (it == out_edges_.end()) {
    return false;
  }

  for (const auto& edge : it->second) {
    if (edge.target_id == to_id) {
      return true;
    }
  }
  return false;
}

double CooccurrenceGraph::GetEdgeWeight(int from_id, int to_id) const {
  auto it = out_edges_.find(from_id);
  if (it == out_edges_.end()) {
    return 0.0;
  }

  for (const auto& edge : it->second) {
    if (edge.target_id == to_id) {
      return edge.tf;
    }
  }
  return 0.0;
}

// ====================================================================================
// SingleWord Implementation
// ====================================================================================

SingleWord::SingleWord(const std::string& unique_term,
                       int id,
                       CooccurrenceGraph* graph)
    : id_(id), unique_term_(unique_term), graph_(graph) {}

SingleWord::~SingleWord() = default;

void SingleWord::AddOccur(const std::string& tag,
                          int sent_id,
                          int pos_sent,
                          int pos_text) {
  // Add occurrence position
  occurs_[sent_id].push_back({pos_sent, pos_text});
  tf_ += 1.0;

  // Update special counters based on tag
  if (tag == "a") {  // Acronym
    tf_a_ += 1.0;
  }
  if (tag == "n") {  // Proper noun
    tf_n_ += 1.0;
  }
}

SingleWord::GraphMetrics SingleWord::GetGraphMetrics() const {
  GraphMetrics metrics;

  // Calculate outgoing edges metrics (right context)
  std::vector<GraphEdge> out_edges = graph_->GetOutEdges(id_);
  metrics.wdr = out_edges.size();
  metrics.wir = 0.0;
  for (const auto& edge : out_edges) {
    metrics.wir += edge.tf;
  }
  metrics.pwr = (metrics.wir == 0.0) ? 0.0 : metrics.wdr / metrics.wir;

  // Calculate incoming edges metrics (left context)
  std::vector<GraphEdge> in_edges = graph_->GetInEdges(id_);
  metrics.wdl = in_edges.size();
  metrics.wil = 0.0;
  for (const auto& edge : in_edges) {
    metrics.wil += edge.tf;
  }
  metrics.pwl = (metrics.wil == 0.0) ? 0.0 : metrics.wdl / metrics.wil;

  return metrics;
}

void SingleWord::UpdateH(const DocumentStats& stats) {
  GraphMetrics graph_metrics = GetGraphMetrics();

  // Calculate wrel using graph-based approach from reference YAKE
  wrel_ = (0.5 + (graph_metrics.pwl * (tf_ / stats.max_tf))) +
          (0.5 + (graph_metrics.pwr * (tf_ / stats.max_tf)));

  // Calculate wfreq (frequency feature)
  wfreq_ = tf_ / (stats.avg_tf + stats.std_tf);

  // Calculate wspread (sentence spread feature)
  wspread_ = static_cast<double>(occurs_.size()) / stats.number_of_sentences;

  // Calculate wcase (case feature)
  wcase_ = std::max(tf_a_, tf_n_) / (1.0 + std::log(tf_));

  // Calculate wpos (position feature) using sentence positions like reference
  if (!occurs_.empty()) {
    std::vector<int> sentence_ids;
    for (const auto& [sent_id, positions] : occurs_) {
      sentence_ids.push_back(sent_id);
    }
    std::sort(sentence_ids.begin(), sentence_ids.end());

    // Use median sentence position
    size_t mid = sentence_ids.size() / 2;
    double median_pos = sentence_ids[mid];
    wpos_ = std::log(std::log(3.0 + median_pos));
  } else {
    wpos_ = std::log(std::log(3.0));
  }

  // Calculate final score using YAKE formula
  h_ = (wpos_ * wrel_) / (wcase_ + (wfreq_ / wrel_) + (wspread_ / wrel_));
}

// ====================================================================================
// ComposedWord Implementation
// ====================================================================================

ComposedWord::ComposedWord(const std::vector<SingleWord*>& terms,
                           const std::string& keyword)
    : terms_(terms), kw_(keyword), size_(terms.size()) {
  unique_kw_ = base::ToLowerASCII(keyword);

  // Check if starts or ends with stopword
  if (!terms_.empty()) {
    start_or_end_stopwords_ =
        terms_.front()->IsStopword() || terms_.back()->IsStopword();
  } else {
    start_or_end_stopwords_ = true;
  }
}

ComposedWord::~ComposedWord() = default;

bool ComposedWord::IsValid() const {
  // A candidate is valid if it doesn't start/end with stopwords
  if (start_or_end_stopwords_) {
    return false;
  }

  // For now, let's be less strict - any non-stopword candidate is valid
  // The reference YAKE has more complex tag validation which we'll implement
  // later
  return true;
}

void ComposedWord::UpdateH() {
  if (terms_.empty()) {
    h_ = 1000.0;  // High score for invalid candidates
    return;
  }

  // Aggregate scores from constituent terms following reference YAKE
  double sum_h = 0.0;
  double prod_h = 1.0;

  for (SingleWord* term : terms_) {
    if (!term->IsStopword()) {
      sum_h += term->GetH();
      prod_h *= term->GetH();
    }
    // Note: Reference YAKE has complex stopword handling with BiWeight
    // For now, we ignore stopwords in scoring
  }

  // Calculate final score
  double tf_used = (tf_ > 0) ? tf_ : 1.0;
  h_ = prod_h / ((sum_h + 1.0) * tf_used);
}

// ====================================================================================
// YakeKeywordExtractor Implementation
// ====================================================================================

YakeKeywordExtractor::YakeKeywordExtractor()
    : graph_(std::make_unique<CooccurrenceGraph>()), next_term_id_(0) {}

YakeKeywordExtractor::~YakeKeywordExtractor() = default;

std::vector<KeywordScore> YakeKeywordExtractor::ExtractKeywords(
    const std::string& text,
    size_t max_keywords,
    size_t max_ngram_size) {
  return ExtractKeywords(text, max_keywords, max_ngram_size,
                         2);  // Default window size of 2
}

std::vector<KeywordScore> YakeKeywordExtractor::ExtractKeywords(
    const std::string& text,
    size_t max_keywords,
    size_t max_ngram_size,
    size_t window_size) {
  if (text.empty()) {
    return {};
  }

  // Clear previous state
  graph_ = std::make_unique<CooccurrenceGraph>();
  terms_.clear();
  candidates_.clear();
  sentences_str_.clear();
  next_term_id_ = 0;

  // Build data structures following reference YAKE pipeline
  BuildDataStructures(text, window_size, max_ngram_size);

  // Build features for single and multi-word terms
  BuildSingleTermFeatures();
  BuildMultiTermFeatures();

  // Collect valid candidates and sort by score (lower is better in YAKE)
  std::vector<std::pair<std::string, double>> candidates;
  for (const auto& [unique_kw, cand] : candidates_) {
    if (cand->IsValid()) {
      candidates.push_back({cand->GetKeyword(), cand->GetH()});
    }
  }

  std::sort(candidates.begin(), candidates.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });

  // Apply deduplication following reference YAKE
  return DeduplicateKeywords(candidates, max_keywords, 0.9);
}

void YakeKeywordExtractor::BuildDataStructures(const std::string& text,
                                               size_t window_size,
                                               size_t n) {
  // Pre-process and tokenize text
  std::string filtered_text = PreFilter(text);
  sentences_str_ = TokenizeSentences(filtered_text);

  int pos_text = 0;

  // Process each sentence
  for (int sentence_id = 0;
       sentence_id < static_cast<int>(sentences_str_.size()); ++sentence_id) {
    ProcessSentence(sentences_str_[sentence_id], sentence_id, pos_text,
                    window_size, n);
  }
}

void YakeKeywordExtractor::ProcessSentence(
    const std::vector<std::string>& sentence,
    int sentence_id,
    int& pos_text,
    size_t window_size,
    size_t n) {
  std::vector<std::pair<std::string, SingleWord*>> block_of_words;

  for (int pos_sent = 0; pos_sent < static_cast<int>(sentence.size());
       ++pos_sent) {
    const std::string& word = sentence[pos_sent];

    // Get linguistic tag for the word
    std::string tag = GetTag(word, pos_sent, kExcludeChars);

    ProcessWord(word, tag, sentence_id, pos_sent, pos_text, block_of_words,
                window_size, n);
  }
}

void YakeKeywordExtractor::ProcessWord(
    const std::string& word,
    const std::string& tag,
    int sentence_id,
    int pos_sent,
    int& pos_text,
    std::vector<std::pair<std::string, SingleWord*>>& block_of_words,
    size_t window_size,
    size_t n) {
  // Skip unusual tokens (digits, mixed alphanumeric, etc.)
  if (tag == "d" || tag == "u") {
    pos_text++;
    return;
  }

  std::string unique_term = ToLowerCase(word);

  // Get or create term
  SingleWord* term_obj = GetOrCreateTerm(unique_term);

  // Set stopword status
  term_obj->SetStopword(IsStopWord(unique_term));

  // Add occurrence information
  term_obj->AddOccur(tag, sentence_id, pos_sent, pos_text);

  // Update co-occurrence relationships
  UpdateCooccurrence(block_of_words, term_obj, window_size);

  // Generate n-gram candidates
  GenerateCandidates(word, tag, term_obj, block_of_words, n);

  // Add to current block
  block_of_words.push_back({tag, term_obj});

  pos_text++;
}

SingleWord* YakeKeywordExtractor::GetOrCreateTerm(
    const std::string& unique_term) {
  auto it = terms_.find(unique_term);
  if (it != terms_.end()) {
    return it->second.get();
  }

  // Create new term
  int term_id = next_term_id_++;
  graph_->AddNode(term_id);

  auto term_obj =
      std::make_unique<SingleWord>(unique_term, term_id, graph_.get());
  SingleWord* result = term_obj.get();
  terms_[unique_term] = std::move(term_obj);

  return result;
}

void YakeKeywordExtractor::UpdateCooccurrence(
    const std::vector<std::pair<std::string, SingleWord*>>& block_of_words,
    SingleWord* current_term,
    size_t window_size) {
  // Calculate window of previous words to consider
  int start = std::max(0, static_cast<int>(block_of_words.size()) -
                              static_cast<int>(window_size));

  for (int i = start; i < static_cast<int>(block_of_words.size()); ++i) {
    const auto& [tag, prev_term] = block_of_words[i];

    // Skip unusual terms
    if (tag == "d" || tag == "u") {
      continue;
    }

    // Add co-occurrence edge
    graph_->IncrementEdgeWeight(prev_term->GetId(), current_term->GetId(), 1.0);
  }
}

void YakeKeywordExtractor::GenerateCandidates(
    const std::string& word,
    const std::string& tag,
    SingleWord* term_obj,
    const std::vector<std::pair<std::string, SingleWord*>>& block_of_words,
    size_t n) {
  // Generate single-word candidate
  std::vector<SingleWord*> single_term = {term_obj};
  auto single_candidate = std::make_unique<ComposedWord>(single_term, word);
  AddOrUpdateComposedWord(std::move(single_candidate));

  // Generate multi-word candidates
  for (size_t ngram_size = 2;
       ngram_size <= n && ngram_size <= block_of_words.size() + 1;
       ++ngram_size) {
    if (block_of_words.size() >= ngram_size - 1) {
      std::vector<SingleWord*> ngram_terms;
      std::vector<std::string> ngram_words;

      // Collect terms for the n-gram
      for (size_t i = block_of_words.size() - (ngram_size - 1);
           i < block_of_words.size(); ++i) {
        const auto& [prev_tag, prev_term] = block_of_words[i];
        if (prev_tag != "d" && prev_tag != "u") {  // Skip unusual terms
          ngram_terms.push_back(prev_term);
          ngram_words.push_back(prev_term->GetUniqueTerm());
        }
      }

      // Add current term
      ngram_terms.push_back(term_obj);
      ngram_words.push_back(word);

      if (ngram_terms.size() == ngram_size) {
        std::string ngram_keyword = base::JoinString(ngram_words, " ");
        auto ngram_candidate =
            std::make_unique<ComposedWord>(ngram_terms, ngram_keyword);
        AddOrUpdateComposedWord(std::move(ngram_candidate));
      }
    }
  }
}

void YakeKeywordExtractor::AddOrUpdateComposedWord(
    std::unique_ptr<ComposedWord> cand) {
  const std::string& unique_kw = cand->GetUniqueKeyword();

  auto it = candidates_.find(unique_kw);
  if (it != candidates_.end()) {
    // Update existing candidate frequency
    it->second->SetTf(it->second->GetTf() + 1.0);
  } else {
    // Add new candidate
    cand->SetTf(1.0);
    candidates_[unique_kw] = std::move(cand);
  }
}

void YakeKeywordExtractor::BuildSingleTermFeatures() {
  DocumentStats stats = CalculateDocumentStats();

  for (const auto& [unique_term, term] : terms_) {
    term->UpdateH(stats);
  }
}

void YakeKeywordExtractor::BuildMultiTermFeatures() {
  for (const auto& [unique_kw, candidate] : candidates_) {
    candidate->UpdateH();
  }
}

DocumentStats YakeKeywordExtractor::CalculateDocumentStats() const {
  DocumentStats stats;
  stats.number_of_sentences = sentences_str_.size();

  // Collect term frequencies
  std::vector<double> term_freqs;
  for (const auto& [unique_term, term] : terms_) {
    double tf = term->GetTf();
    term_freqs.push_back(tf);
    stats.max_tf = std::max(stats.max_tf, tf);
    stats.number_of_words += static_cast<int>(tf);
  }

  if (!term_freqs.empty()) {
    // Calculate mean
    stats.avg_tf = std::accumulate(term_freqs.begin(), term_freqs.end(), 0.0) /
                   term_freqs.size();

    // Calculate standard deviation
    double sum_sq_diff = 0.0;
    for (double tf : term_freqs) {
      sum_sq_diff += (tf - stats.avg_tf) * (tf - stats.avg_tf);
    }
    stats.std_tf = std::sqrt(sum_sq_diff / term_freqs.size());
  }

  return stats;
}

// ====================================================================================
// Text Processing Utilities
// ====================================================================================

std::vector<std::vector<std::string>> YakeKeywordExtractor::TokenizeSentences(
    const std::string& text) {
  // Simple sentence splitting and tokenization
  // This is a basic implementation - reference YAKE uses segtok library

  std::vector<std::vector<std::string>> sentences;
  std::vector<std::string> sentence_strings = base::SplitString(
      text, ".!?", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  for (const std::string& sentence_str : sentence_strings) {
    std::vector<std::string> tokens =
        base::SplitString(sentence_str, " \t\n\r", base::TRIM_WHITESPACE,
                          base::SPLIT_WANT_NONEMPTY);

    if (!tokens.empty()) {
      sentences.push_back(tokens);
    }
  }

  return sentences;
}

std::string YakeKeywordExtractor::PreFilter(const std::string& text) {
  // Basic pre-filtering similar to reference YAKE
  std::string result = text;
  std::replace(result.begin(), result.end(), '\n', ' ');
  std::replace(result.begin(), result.end(), '\t', ' ');
  return result;
}

std::string YakeKeywordExtractor::GetTag(const std::string& word,
                                         int position,
                                         const std::set<char>& exclude) {
  // Linguistic tagging following reference YAKE get_tag function

  // Filter out tokens shorter than 3 characters
  if (word.length() < 3) {
    return "u";
  }

  // Check if word is numeric
  bool is_numeric = true;
  for (char c : word) {
    if (!std::isdigit(c) && c != ',' && c != '.') {
      is_numeric = false;
      break;
    }
  }
  if (is_numeric) {
    return "d";
  }

  // Count character types
  int digit_count = 0;
  int alpha_count = 0;
  int exclude_count = 0;

  for (char c : word) {
    if (std::isdigit(c)) {
      digit_count++;
    } else if (std::isalpha(c)) {
      alpha_count++;
    } else if (exclude.find(c) != exclude.end()) {
      exclude_count++;
    }
  }

  // Check for unusual patterns
  if ((digit_count > 0 && alpha_count > 0) ||
      (digit_count == 0 && alpha_count == 0) || exclude_count > 1) {
    return "u";
  }

  // Check for acronym (all uppercase)
  if (word.length() > 0 && std::all_of(word.begin(), word.end(), [](char c) {
        return std::isupper(c) || !std::isalpha(c);
      })) {
    return "a";
  }

  // Check for proper noun (capitalized, not at start of sentence)
  if (word.length() > 1 && std::isupper(word[0]) && position > 0) {
    bool only_first_upper =
        std::all_of(word.begin() + 1, word.end(),
                    [](char c) { return std::islower(c) || !std::isalpha(c); });
    if (only_first_upper) {
      return "n";
    }
  }

  return "p";  // Plain word
}

std::vector<KeywordScore> YakeKeywordExtractor::DeduplicateKeywords(
    const std::vector<std::pair<std::string, double>>& candidates,
    size_t max_keywords,
    double dedup_threshold) {
  std::vector<KeywordScore> result;

  for (const auto& [keyword, score] : candidates) {
    bool should_add = true;

    // Check similarity with existing results
    for (const auto& existing : result) {
      if (CalculateStringSimilarity(keyword, existing.keyword) >
          dedup_threshold) {
        should_add = false;
        break;
      }
    }

    if (should_add) {
      result.push_back({keyword, score});
    }

    if (result.size() >= max_keywords) {
      break;
    }
  }

  return result;
}

double YakeKeywordExtractor::CalculateStringSimilarity(
    const std::string& str1,
    const std::string& str2) {
  // Levenshtein similarity following reference YAKE
  if (str1.empty() && str2.empty()) {
    return 1.0;
  }
  if (str1.empty() || str2.empty()) {
    return 0.0;
  }

  size_t len1 = str1.length();
  size_t len2 = str2.length();

  std::vector<std::vector<int>> dp(len1 + 1, std::vector<int>(len2 + 1));

  for (size_t i = 0; i <= len1; ++i) {
    dp[i][0] = i;
  }
  for (size_t j = 0; j <= len2; ++j) {
    dp[0][j] = j;
  }

  for (size_t i = 1; i <= len1; ++i) {
    for (size_t j = 1; j <= len2; ++j) {
      if (str1[i - 1] == str2[j - 1]) {
        dp[i][j] = dp[i - 1][j - 1];
      } else {
        dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
      }
    }
  }

  int distance = dp[len1][len2];
  int max_len = std::max(len1, len2);
  return 1.0 - static_cast<double>(distance) / max_len;
}

bool YakeKeywordExtractor::IsStopWord(const std::string& word) {
  return kStopWords.find(ToLowerCase(word)) != kStopWords.end();
}

std::string YakeKeywordExtractor::ToLowerCase(const std::string& text) {
  return base::ToLowerASCII(text);
}

}  // namespace local_ai
