
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include "../aux.hpp"
#include "hmm.hpp"
#include "report.hpp"
#include "../timer.hpp"
#include "../plasma/find.hpp"

using namespace std;

void check_data(const Data::Collection &collection, const hmm_options &options)
{
  if(options.verbosity >= Verbosity::info)
    for(auto &contrast: collection) {
      cout << "Data collection has " << contrast.set_size << " sequences with a total size of " << contrast.seq_size << " nucleotides." << endl;
      for(auto &dataset: contrast) {
        cout << dataset.path << " has " << dataset.set_size << " sequences with a total size of " << dataset.seq_size << " nucleotides." << endl;
        if(options.verbosity >= Verbosity::verbose) {
          cout << "The SHA1 of this set is " << dataset.sha1 << endl;
          cout << "The first 3 sequences are:" << endl;
          size_t i = 0;
          for(auto &seq: dataset) {
            cout << ">" << seq.definition << endl << seq.sequence << endl;
            if(++i >= 3) break;
          }
        }
        if(options.verbosity >= Verbosity::debug)
          for(auto &seq: dataset)
            cout << ">" << seq.definition << endl << seq.sequence << endl;
      }
    }

// TODO re-enable warning
//  if(options.training_method != Training::Method::none and not is_generative(options.objective.measure) and options.objective.measure != Measure::rank_information) {
//    for(auto &contrast: collection) {
//      if(contrast.sets.size() < 2) {
//        cout << "Please note that for discriminative training you need to specify multiple sequence sets using -f." << endl;
//        exit(-1);
//      }
//    }
//  }
}


void train_evaluate(HMM &hmm, const Data::Collection &all_data, const Data::Collection &training_data, const Data::Collection &test_data, const hmm_options &options, bool do_training, bool relearning_phase=false)
{
  // Define the learning and evaluation tasks
  Training::Tasks eval_tasks = hmm.define_training_tasks(options);

  if(do_training) {
    // define the training tasks
    Training::Tasks learn_tasks = hmm.define_training_tasks(options);

    if(relearning_phase) {
      switch(options.relearning) {
        case Relearning::None:
          learn_tasks = Training::Tasks();
          break;
        case Relearning::Full:
          break;
        // case Relearning::Added:
        // TODO implement
        //   break;
        case Relearning::Reestimation:
          {
            set<string> contrast_names;
            for(auto &task: eval_tasks)
              for(auto &expr: task)
                contrast_names.insert(expr.contrast);

            learn_tasks = Training::Tasks();
            Training::Task task;
            task.motif_name = "Background";
            task.measure = Measure::Likelihood;
            for(auto &contrast_name: contrast_names)
              task.contrast_expression.push_back({+1, contrast_name});

            task.targets.emission.push_back(1);
            for(size_t i = 0; i < hmm.get_nstates(); i++)
              task.targets.transition.push_back(i);

            if(options.verbosity >= Verbosity::verbose) {
              cout << "Generated generative training targets for re-learning." << endl << "Emissions:";
              for(auto e: task.targets.emission)
                cout << " " << e;
              cout << endl;
              cout << "Transitions:";
              for(auto t: task.targets.transition)
                cout << " " << t;
              cout << endl;
            }
            learn_tasks.push_back(task);
          }
      }
    }

    if(not learn_tasks.empty())
      hmm.train(training_data, learn_tasks, options);
  }

  if(test_data.set_size != 0) {
    evaluate_hmm(hmm, training_data, "training", eval_tasks, options);
    evaluate_hmm(hmm, test_data, "Test", eval_tasks, options);
  }
  evaluate_hmm(hmm, all_data, "", eval_tasks, options);
}

double calc_expected_seq_size(const Data::Collection &collection)
{
  return(1.0 * collection.seq_size / collection.set_size);
}

string padding(size_t n, char c='n') {
  string s;
  for(size_t i = 0; i < n; i++)
    s += c;
  return(s);
}

/* FIXME remove
string pad(const string &s, size_t l, size_t r, char c='n') {
  return(padding(l,c) + s + padding(r,c));
}  */

vector<string> generate_wiggle_variants(const string &s,  size_t n, Verbosity verbosity) {
  if(verbosity >= Verbosity::debug)
    cout << "generate_wiggle_variants(" << s << ", " << n << ")" << endl;
  size_t l = s.size();
  vector<string> variants;
  variants.push_back(s);
  for(size_t i = 1; i <= min<size_t>(l, n); i++) {
    variants.push_back(padding(i) + s.substr(0, l - i));
    variants.push_back(s.substr(i, l - i) + padding(i));
  }
  if(verbosity >= Verbosity::debug)
    for(auto &x: variants)
      cout << "Wiggle variant: " << x << endl;
  return(variants);
}


HMM doit(const Data::Collection &all_data, const Data::Collection &training_data, const Data::Collection &test_data, const hmm_options &options_)
{
  // potentially: regardless of the objective function chosen for training, one might use the MICO p-value for selection!
  const bool use_mico_pvalue = true; // whether to use MICO p-value in multiple motif mode
  const bool drop_below_mico_pvalue_threshold = true; // whether to drop models below MICO p-value threshold in multiple motif mode
  const double p_mico_threshold = -log(0.05); // MICO p-value threshold to drop models in multiple motif mode

  hmm_options options = options_;

  if(options.verbosity >= Verbosity::debug)
    cout << "About to construct HMM." << endl;

  // initialize HMM
  HMM hmm(options.verbosity, options.contingency_pseudo_count);

  // load HMM from file if specified
  size_t n_loaded = 0;
  bool training_necessary = false;
  for(auto &load_path: options.load_paths)
    if(n_loaded++ == 0)
      hmm = HMM(load_path, options.verbosity, options.contingency_pseudo_count);
    else {
      hmm.add_motifs(HMM(load_path, options.verbosity, options.contingency_pseudo_count));
      training_necessary = true;
    }

  if(options.verbosity >= Verbosity::verbose)
    cout << "Model after initialization = " << hmm << endl;

  hmm.switch_intermediate(options.store_intermediate);

  // train background
  if(n_loaded == 0 and not options.objectives.empty()) {
    hmm.initialize_bg_with_bw(training_data, options);

    if(options.verbosity >= Verbosity::verbose)
      cout << "Model after background learning = " << hmm << endl;
  }

  double expected_seq_size = calc_expected_seq_size(all_data);
  if(options.verbosity >= Verbosity::info)
    cout << "The average sequence size is " << expected_seq_size << "nt." << endl;

  for(auto &spec: options.motif_specifications)
    switch(spec.kind) {
      case Specification::Motif::Kind::File:
        // load emission matrix and add it.
        hmm.add_motif(read_emission(spec.specification), expected_seq_size, options.lambda, spec.name, spec.insertions, options.left_padding, options.right_padding);
        break;
      case Specification::Motif::Kind::Seed:
        // use IUPAC regular expression
        hmm.add_motif(spec.specification, options.alpha, expected_seq_size, options.lambda, spec.name, spec.insertions, options.left_padding, options.right_padding);
        training_necessary = true;
        break;
      case Specification::Motif::Kind::Plasma:
        options.seeding.motif_specifications.push_back(spec);
        training_necessary = true;
        break;
    }

  if(find_if(begin(options.motif_specifications), end(options.motif_specifications), [](const Specification::Motif &motif) {
        return(motif.kind == Specification::Motif::Kind::Plasma);
        }) == end(options.motif_specifications)) {
    if(options.verbosity >= Verbosity::info)
      cout << "No automatic seeds are used." << endl;
    train_evaluate(hmm, all_data, training_data, test_data, options, training_necessary);
  } else {
    if(options.verbosity >= Verbosity::info)
      cout << "Determining seeds automatically." << endl;

    Seeding::Collection collection(training_data);

    if(options.verbosity >= Verbosity::debug) {
      for(auto &contrast: training_data)
        for(auto &dataset: contrast) {
          cerr << "HMM::Contrast " << contrast.name << " set -> motifs:";
          for(auto &m: dataset.motifs)
            cerr << " " << m;
          cerr << endl;
        }
    }
    if(options.verbosity >= Verbosity::debug) {
      for(auto &contrast: collection)
        for(auto &dataset: contrast) {
          cerr << "Seeding::Contrast " << contrast.name << " set -> motifs:";
          for(auto &m: dataset.motifs)
            cerr << " " << m;
          cerr << endl;
        }
    }

    Seeding::Plasma plasma(collection, options.seeding);

    if(options.verbosity >= Verbosity::debug)
      cout << "motif_specs.size() = " << plasma.options.motif_specifications.size() << endl;
    size_t plasma_motif_idx = 0;

    // while there are motif specifications left
    while(plasma_motif_idx < plasma.options.motif_specifications.size()) {

      if(hmm.get_nmotifs() > 0) {
        cout << "Masking plasma data collection." << endl;
        plasma.collection.mask(hmm.compute_mask(training_data));
      }

      // consider the next motif specification
      auto motif_spec = options.motif_specifications[plasma_motif_idx];

      // determine the matching Plasma objective
      auto objectives = Training::corresponding_objectives(options.objectives, options.use_mi_to_seed);
      plasma.options.objectives = objectives;
      if(options.verbosity >= Verbosity::info) {
        cout << "Determining seed " << motif_spec << " with objective functions";
        for(auto &obj: objectives)
          cout << " " << obj;
        cout << "." << endl;
      }

      // find the next set of seeds with Plasma
      Seeding::Results plasma_results = plasma.find(motif_spec, objectives);

      // if no seeds are found, don't try to find more seeds
      if(plasma_results.empty())
        break;
      if(options.verbosity >= Verbosity::debug)
        cout << motif_spec.name << " result.size() = " << plasma_results.size() << endl;

      vector<pair<string, HMM>> learned_models;

      // seed and learn HMM parameters independently for each Plasma motif
      for(size_t seed_idx = 0; seed_idx < plasma_results.size(); seed_idx++) {
        string motif = plasma_results[seed_idx].motif;
        string name = motif_spec.name;

        cout << "Considering candidate motif " << name << ":" << motif << " and training to determine the HMM score." << endl;

        plasma_results[seed_idx].score = -numeric_limits<double>::infinity();

        // consider all wiggle variants
        for(auto variant: generate_wiggle_variants(motif, options.wiggle, options.verbosity)) {
          HMM model(hmm);
          if(options.verbosity >= Verbosity::info and options.wiggle > 0)
            cout << "Considering wiggle variant " << variant << " of candidate motif " << name << ":" << motif << " and training to determine the HMM score." << endl;
          model.add_motif(variant, options.alpha, expected_seq_size, options.lambda, name, plasma.options.motif_specifications[plasma_motif_idx].insertions, options.left_padding, options.right_padding);

          hmm_options options_(options);
          if(options_.long_names)
            options_.label += "." + variant;
          train_evaluate(model, all_data, training_data, test_data, options_, training_necessary);

          learned_models.push_back(make_pair(variant, model));
        }
      }

      if(not options.accept_multiple) {
        if(learned_models.size() == 1)
          hmm = learned_models.begin()->second;
        else {
          if(options.verbosity >= Verbosity::info)
            cout << "Evaluating learned models.";
          auto best_model = hmm;
          string best_seed = "";
          double best_score = -numeric_limits<double>::infinity();
          for(auto &learned: learned_models) {
            string seed = learned.first;
            auto model = learned.second;
            Training::Tasks tasks = model.define_training_tasks(options);
            // TODO consider using compute_score instead of compute_score_all_motifs
            double score = model.compute_score_all_motifs(training_data, tasks.begin()->measure, options.weighting);
            if(score > best_score) {
              best_score = score;
              best_seed = seed;
              best_model = model;
            }
          }
          if(best_score >= -numeric_limits<double>::infinity()) {
            if(options.verbosity >= Verbosity::info)
              cout << "Accepting seed " << best_seed << " with score " << best_score << endl;
            hmm = best_model;
          } else
            if(options.verbosity >= Verbosity::info)
              cout << "We did not find an acceptable model." << endl;
        }
      } else {
        bool ok = true;

        vector<size_t> absent_groups;
        while(ok and not learned_models.empty()) {
          auto data = training_data;

          size_t best_index = 0;
          auto best_model = hmm;
          string best_seed = "";
          bool updated = false;
          double best_score = -numeric_limits<double>::infinity();

          size_t index = 0;

          vector<string> below_threshold;

          for(auto &learned: learned_models) {
            string seed = learned.first;
            auto learned_model = learned.second;

            auto model = hmm;
            model.add_motifs(learned_model, false);

            // TODO adapt transition probabilities?
            // TODO or do complete relearning?

            // motif_len, n, and df are needed for MICO p-value computation
            const size_t motif_len = seed.size();
            const double n = data.set_size; // TODO fix this with regards to pseudo counts and exact reference to the relevant contrast
            double df = 0;
            for(auto &contrast: data)
              df += (contrast.sets.size() - 1); // TODO only use the relevant contrasts for this motif

            vector<size_t> groups_to_score = {model.get_ngroups() - 1}; // only add the most recently added group

            double score = -numeric_limits<double>::infinity();

            if(not use_mico_pvalue)
              // TODO actually select the specific task - above we do fix all conceivable tasks...
              score = model.compute_score(data, model.define_training_tasks(options).begin()->measure, options.weighting, groups_to_score, absent_groups);
            else {
              if(absent_groups.empty()) {
                score = model.compute_score(data, Measures::Continuous::Measure::MutualInformation, options.weighting, groups_to_score, vector<size_t>());
                cout << "mi = " << score << endl;
              } else {
                cout << "Scoring residual information - motifs:" << endl;

                if(not options.revcomp) {

                  if(options.verbosity >= Verbosity::info)
                  for(size_t i = 0; i < model.get_ngroups(); i++)
                    if(model.is_motif_group(i))
                      cout << (find(begin(groups_to_score), end(groups_to_score), i) != end(groups_to_score) ? "+" : "-")
                        << " " << model.get_group_consensus(i) << endl;

                  score = model.compute_score(data, Measures::Continuous::Measure::ResidualMutualInformation, options.weighting, groups_to_score, vector<size_t>(), absent_groups);

                } else {

                  if(options.verbosity >= Verbosity::info)
                    cout << "Adding reverse complementary motifs." << endl;

                  pair<HMM, map<size_t,size_t>> rc = model.add_revcomp_motifs();
                  HMM rc_model = rc.first;
                  map<size_t,size_t> rc_assoc = rc.second;

                  auto rc_groups_to_score = groups_to_score;
                  for(auto x: groups_to_score)
                    rc_groups_to_score.push_back(rc_assoc[x]);

                  auto rc_absent_groups = absent_groups;
                  for(auto x: absent_groups)
                    rc_absent_groups.push_back(rc_assoc[x]);

                  if(options.verbosity >= Verbosity::info)
                    for(size_t i = 0; i < rc_model.get_ngroups(); i++)
                      if(rc_model.is_motif_group(i))
                        cout << (find(begin(rc_groups_to_score), end(rc_groups_to_score), i) != end(rc_groups_to_score) ? "+" : "-")
                          << " " << rc_model.get_group_consensus(i) << endl;

                  score = rc_model.compute_score(data, Measures::Continuous::Measure::ResidualMutualInformation, options.weighting, rc_groups_to_score, vector<size_t>(), rc_absent_groups);

                }

                cout << "residual mutual information = " << score << endl;
              }

              if(score > -numeric_limits<double>::infinity())
                score = - corrected_pvalue(score, n, df, motif_len, Verbosity::verbose);
              cout << "score = " << score << endl;
            }

            cout << "Score of the model augmented by " << seed << " has a score of " << score << endl << endl;
            if(not (use_mico_pvalue and drop_below_mico_pvalue_threshold) or score >= p_mico_threshold) {
              if(score > best_score) {
                cout << "This is the currently best model!" << endl << endl;
                ok = true;
                updated = true;
                best_model = model;
                best_seed = seed;
                best_score = score;
                best_index = index;
              }
            } else
              below_threshold.push_back(seed);
            index++;
          }

          if(not updated) {
            cout << "We did not find an improved model." << endl;
            ok = false;
          } else {
            hmm = best_model;

            if(options.verbosity >= Verbosity::info)
              cout << "Accepting seed " << best_seed << " with score " << best_score << endl;

            hmm_options options_(options);
            if(options.long_names)
              options.label += "." + best_seed;

            // TODO: remember that learning and evaluation is to a large degree based on groups - not motif names; thus there is some inefficiencies
            // TODO: learning should perhaps only adapt transitions? this would be very fast...
            // TODO:   otherwise: if learning should also adapt emissions, then perhaps only those of the new motif?
            // TODO:   in this case: it might be done on the masked sequences
            train_evaluate(hmm, all_data, training_data, test_data, options, training_necessary, true);

            absent_groups.push_back(hmm.get_ngroups() - 1);

            learned_models.erase(begin(learned_models) + best_index);
            ok = true;
          }

          if(options.verbosity >= Verbosity::info) {
            cout << "To be removed:";
            for(auto &mod: below_threshold)
              cout << " " << mod;
            cout << endl;
          }

          auto is_below_threshold = [&below_threshold] (const pair<string, HMM> &x) {
            return(find(begin(below_threshold), end(below_threshold), x.first) != end(below_threshold));
          };

          if(options.verbosity >= Verbosity::info) {
            cout << "Before removal:";
            for(auto &mod: learned_models)
              cout << " " << mod.first;
            cout << endl;
            cout << "Before removal " << learned_models.size() << " remaining." << endl;
          }

          if(drop_below_mico_pvalue_threshold)
            learned_models.erase(remove_if(begin(learned_models), end(learned_models), is_below_threshold),
                end(learned_models));

          if(options.verbosity >= Verbosity::info) {
            cout << "After removal:";
            for(auto &mod: learned_models)
              cout << " " << mod.first;
            cout << endl;
            cout << "After removal " << learned_models.size() << " remaining." << endl;
          }
        }
      }

      plasma_motif_idx++;
    }
  }
  return(hmm);
}

vector<HMM> cross_validation(const Data::Collection &all_data, const hmm_options &options)
{
  vector<HMM> hmms;
  for(size_t cross_validation_iteration = 0; cross_validation_iteration < options.cross_validation_iterations; cross_validation_iteration++) {
    if(options.verbosity >= Verbosity::info and options.cross_validation_iterations > 1)
      cout << "Doing cross-validation " << (cross_validation_iteration + 1) << " of " << options.cross_validation_iterations << "." << endl;

    hmm_options opt = options;

    if(options.cross_validation_freq < 1)
      opt.label += ".cv" + boost::lexical_cast<string>(cross_validation_iteration);

    Data::Collection training_data, test_data;
    prepare_cross_validation(all_data, training_data, test_data, options.cross_validation_freq, options.verbosity);
    HMM hmm = doit(all_data, training_data, test_data, opt);
    hmms.push_back(hmm);
  }
  return(hmms);
}

void perform_analysis(hmm_options &options)
{
  if(options.verbosity >= Verbosity::verbose)
    cout << "Loading sequences." << endl;

  Data::Collection collection(options.paths, options.revcomp, options.n_seq);

  check_data(collection, options);

  if(options.cross_validation_iterations == 0 or options.cross_validation_freq == 1) {
    options.cross_validation_freq = 1;
    options.cross_validation_iterations = 1;
  }
  vector<HMM> hmms = cross_validation(collection, options);
}

