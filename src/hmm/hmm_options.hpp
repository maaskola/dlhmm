/* =====================================================================================
 * Copyright (c) 2011, Jonas Maaskola
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * =====================================================================================
 *
 *       Filename:  hmm_options.hpp
 *
 *    Description:  Data type for options to train and use HMM
 *
 *        Created:  Thu Aug 4 22:12:31 2011 +0200
 *
 *         Author:  Jonas Maaskola <jonas@maaskola.de>
 *
 * =====================================================================================
 */

#ifndef HMM_OPTIONS_HPP
#define HMM_OPTIONS_HPP

#include <string>
#include <vector>
#include "../plasma/options.hpp"
#include "basedefs.hpp"
#include "../executioninformation.hpp"
#include "../plasma/specification.hpp"

namespace Options {
enum class Compression { none, gzip, bzip2 };

std::string compression2string(Compression compression);
std::string compression2ending(Compression compression);

struct Sampling {
  bool do_sampling;  // whether to perform Gibbs sampling learning
  int min_size;
  int max_size;
  double temperature;
  double anneal_factor;
  size_t n_indels;
  size_t n_shift;
  size_t n_parallel;
};

struct Termination {
  size_t max_iter, past;
  double gamma_tolerance;
  double delta_tolerance;
  double epsilon_tolerance;
  bool absolute_improvement;
};

struct LineSearch {
  double mu;
  double eta;
  double delta = 0.66;
  size_t max_steps;
};

struct Evaluation {
  bool conditional_motif_probability;
  bool skip_occurrence_table;
  bool skip_summary;
  bool skip_viterbi_path;
  bool skip_bed;
  bool perform_ric;
  bool print_posterior;
};

struct Conjugate {
  enum class Mode {
    None,
    FletcherReeves,
    PolakRibiere,
    HestenesStiefel,
    DaiYuan
  };
  Mode mode;
  size_t restart_iteration;
  double restart_threshold;
};

struct MultiMotif {
  enum class Relearning {
    None,          // Just add, no relearning
    Reestimation,  // Adapt transitions and background emissions
    Full           // Re-train both previous and newly added motifs
    // TODO: one might want to add the following modes to train only the most
    // recently added motif:
    //   * using conditional MI
    //   * using the motif's objective
  };

  bool accept_multiple;
  Relearning relearning;
  double residual_ratio;
};

struct HMM {
  std::vector<Specification::Set> paths;
  Specification::Motifs motif_specifications;
  std::vector<std::string> load_paths;
  std::string label;
  bool dont_save_shuffle_sequences;
  std::vector<std::string> seeds;
  Seeding::Options seeding;
#if CAIRO_FOUND
  Logo::Options logo;
#endif
  Evaluation evaluate;
  size_t n_threads;
  size_t n_seq;
  double alpha;
  double contingency_pseudo_count, emission_pseudo_count,
      transition_pseudo_count;
  size_t n_simulations;
  bool class_model;
  bool revcomp;
  bool weighting;
  MultiMotif multi_motif;
  Compression output_compression;
  bool self_transition;
  size_t extend;
  size_t left_padding, right_padding;
  bool timing_information;
  size_t cross_validation_iterations;
  double cross_validation_freq;
  bool store_intermediate;  // to write out intermediate parameterizations
  size_t wiggle;
  Conjugate conjugate;
  LineSearch line_search;
  unsigned int random_salt;  // seed for the random number generator

  bool dont_learn_class_prior;
  bool dont_learn_conditional_motif_prior;
  double class_prior;
  double conditional_motif_prior1, conditional_motif_prior2;

  Training::Method bg_learning;
  Training::Objectives objectives;

  Termination termination;

  bool limit_logp;  // whether to report min(0,corrected logp) or just corrected
                    // logp)
  bool use_mi_to_seed;

  Sampling sampling;

  double lambda;
  std::vector<std::string> emission_matrix_paths;

  Verbosity verbosity;
  ExecutionInformation exec_info;
};

std::istream &operator>>(std::istream &is, Compression &type);
std::istream &operator>>(std::istream &is, MultiMotif::Relearning &relearning);
std::istream &operator>>(std::istream &is, Conjugate::Mode &conjugate);

std::ostream &operator<<(std::ostream &os, const Compression &type);
std::ostream &operator<<(std::ostream &os,
                         const MultiMotif::Relearning &relearning);
std::ostream &operator<<(std::ostream &os, const Verbosity &verbosity);
std::ostream &operator<<(std::ostream &os, const LineSearch &options);
std::ostream &operator<<(std::ostream &os, const Termination &options);
std::ostream &operator<<(std::ostream &os, const Sampling &options);
std::ostream &operator<<(std::ostream &os,
                         const ExecutionInformation &exec_info);
std::ostream &operator<<(std::ostream &os, const HMM &options);
};

namespace Exception {
namespace HMM {
struct InvalidCompression : public std::runtime_error {
  InvalidCompression(const std::string &token);
};
struct InvalidRelearning : public std::runtime_error {
  InvalidRelearning(const std::string &token);
};
}
namespace Optimization {
struct InvalidConjugate : public std::runtime_error {
  InvalidConjugate(const std::string &token);
};
}
}

#endif
