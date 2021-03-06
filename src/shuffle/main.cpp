/*
 * =====================================================================================
 *
 *       Filename:  main.cpp
 *
 *    Description:  A tool to generate shuffles of sequences
 *
 *        Created:  19.11.2013 20:54:24
 *         Author:  Jonas Maaskola <jonas@maaskola.de>
 *
 * =====================================================================================
 */
#include <cstdlib>
#include <fstream>
#include <cstdlib>
#include <cstddef>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <vector>
#include "../random_distributions.hpp"
#include "../verbosity.hpp"
#include "../plasma/fasta.hpp"
#include "../plasma/io.hpp"
#include "dinucleotide_shuffle.hpp"
#include <git_config.hpp>

const std::string program_name = "discrover-shuffle";

std::string gen_usage_string() {
  const std::string usage = "Generates dinucleotide frequency preserving shuffles of FASTA files.\n"
    "\n"
    "If no paths are given, sequences are read from standard input.\n"
    "\n"
    "The code is based on altschulEriksonDinuclShuffle.py by P. Clote, from Oct 2003.\n";
  return usage;
}

using namespace std;

void shuffle(istream &is, size_t n, size_t seed) {
  mt19937 rng;
  rng.seed(seed);
  auto parsing = [&n, &seed, &rng](Fasta::Entry &&entry) {
    for (size_t i = 0; i < n; i++) {
      string seq = entry.sequence;
      for (auto &s : seq) {
        s = tolower(s);
        if (s == 'u')
          s = 't';
      }
      cout << ">" << entry.definition << endl
           << dinucleotideShuffle(seq, RandomDistribution::Uniform(rng)) << endl;
    }
    return true;
  };
  auto parser = Fasta::make_parser(parsing);
  is >> parser;
}

int main(int argc, const char **argv) {
  vector<string> paths;
  size_t n = 1;
  size_t seed = 1;
  Verbosity verbosity = Verbosity::info;

  namespace po = boost::program_options;

  // Declare the supported options.
  po::options_description desc("Options");
  try {
    desc.add_options()
      ("help,h", "produce help message")
      ("version", "Print out the version. Also show git SHA1 with -v.")
      ("fasta,f", po::value(&paths),
       "Path of a FASTA file. "
       "May be given multiple times. "
       "Note: usage of -f / --fasta is optional; all free arguments are taken to be paths of FASTA files."
      )
      ("number,n", po::value(&n)->default_value(1), "How many shuffles to generate per sequence.")
      ("seed,s", po::value(&seed), "Seed to initialize random number generator.")
      ("verbose,v", "Be verbose about the progress")
      ;
  } catch (...) {
    cout << "Error while generating command line options." << endl
         << "Please notify the developers." << endl;
    return EXIT_FAILURE;
  }
 
  po::positional_options_description pos;
  pos.add("fasta", -1);

  po::variables_map vm;

  try {
    po::store(
        po::command_line_parser(argc, argv).options(desc).positional(pos).run(),
        vm);
  } catch (po::unknown_option &e) {
    cout << "Error while parsing command line options:" << endl << "Option "
         << e.get_option_name() << " not known." << endl
         << "Please inspect the command line help with -h or --help." << endl;
    return EXIT_FAILURE;
  } catch (po::ambiguous_option &e) {
    cout << "Error while parsing command line options:" << endl << "Option "
         << e.get_option_name() << " is ambiguous." << endl
         << "Please inspect the command line help with -h or --help." << endl;
    return EXIT_FAILURE;
  } catch (po::multiple_values &e) {
    cout << "Error while parsing command line options:" << endl << "Option "
         << e.get_option_name() << " was specified multiple times." << endl
         << "Please inspect the command line help with -h or --help." << endl;
    return EXIT_FAILURE;
  } catch (po::multiple_occurrences &e) {
    cout << "Error while parsing command line options:" << endl << "Option --"
         << e.get_option_name() << " was specified multiple times." << endl
         << "Please inspect the command line help with -h or --help." << endl;
    return EXIT_FAILURE;
  } catch (po::invalid_option_value &e) {
    cout << "Error while parsing command line options:" << endl
         << "The value specified for option " << e.get_option_name()
         << " has an invalid format." << endl
         << "Please inspect the command line help with -h or --help." << endl;
    return EXIT_FAILURE;
  } catch (po::too_many_positional_options_error &e) {
    cout << "Error while parsing command line options:" << endl
         << "Too many positional options were specified." << endl
         << "Please inspect the command line help with -h or --help." << endl;
    return EXIT_FAILURE;
  } catch (po::invalid_command_line_syntax &e) {
    cout << "Error while parsing command line options:" << endl
         << "Invalid command line syntax." << endl
         << "Please inspect the command line help with -h or --help." << endl;
    return EXIT_FAILURE;
  } catch (po::invalid_command_line_style &e) {
    cout << "Error while parsing command line options:" << endl
         << "There is a programming error related to command line style."
         << endl << "Please inspect the command line help with -h or --help."
         << endl;
    return EXIT_FAILURE;
  } catch (po::reading_file &e) {
    cout << "Error while parsing command line options:" << endl
         << "The configuration file can not be read." << endl
         << "Please inspect the command line help with -h or --help." << endl;
    return EXIT_FAILURE;
  } catch (po::validation_error &e) {
    cout << "Error while parsing command line options:" << endl
         << "Validation of option " << e.get_option_name() << " failed." << endl
         << "Please inspect the command line help with -h or --help." << endl;
    return EXIT_FAILURE;
  } catch (po::error &e) {
    cout << "Error while parsing command line options:" << endl
         << "No further information as to the nature of this error is "
            "available, please check your command line arguments." << endl
         << "Please inspect the command line help with -h or --help." << endl;
    return EXIT_FAILURE;
  }

  if (vm.count("verbose"))
    verbosity = Verbosity::verbose;

  if (vm.count("version") and not vm.count("help")) {
    cout << program_name << " " << GIT_DESCRIPTION << " [" << GIT_BRANCH
         << " branch]" << endl;
    if (verbosity >= Verbosity::verbose)
      cout << GIT_SHA1 << endl;
    return EXIT_SUCCESS;
  }

  if (vm.count("help")) {
    cout << program_name << " " << GIT_DESCRIPTION << endl
         << "Copyright (C) 2013 Jonas Maaskola\n"
            "Provided under GNU General Public License Version 3 or later.\n"
            "See the file COPYING provided with this software for details of "
            "the license.\n" << endl;
    cout << gen_usage_string() << endl;
    cout << desc << "\n";
    return 1;
  }

  try {
    po::notify(vm);
  } catch (po::multiple_values &e) {
    cout << "Error while parsing command line options:" << endl << "Option "
         << e.get_option_name() << " was specified multiple times." << endl
         << "Please inspect the command line help with -h or --help." << endl;
    return EXIT_FAILURE;
  } catch (po::invalid_option_value &e) {
    cout << "Error while parsing command line options:" << endl
         << "The value specified for option " << e.get_option_name()
         << " has an invalid format." << endl
         << "Please inspect the command line help with -h or --help." << endl;
    return EXIT_FAILURE;
  } catch (po::too_many_positional_options_error &e) {
    cout << "Error while parsing command line options:" << endl
         << "Too many positional options were specified." << endl
         << "Please inspect the command line help with -h or --help." << endl;
    return EXIT_FAILURE;
  } catch (po::invalid_command_line_syntax &e) {
    cout << "Error while parsing command line options:" << endl
         << "Invalid command line syntax." << endl
         << "Please inspect the command line help with -h or --help." << endl;
    return EXIT_FAILURE;
  } catch (po::invalid_command_line_style &e) {
    cout << "Error while parsing command line options:" << endl
         << "There is a programming error related to command line style."
         << endl << "Please inspect the command line help with -h or --help."
         << endl;
    return EXIT_FAILURE;
  } catch (po::reading_file &e) {
    cout << "Error while parsing command line options:" << endl
         << "The configuration file can not be read." << endl
         << "Please inspect the command line help with -h or --help." << endl;
    return EXIT_FAILURE;
  } catch (po::required_option &e) {
    cout << "Error while parsing command line options:" << endl
         << "The required option " << e.get_option_name()
         << " was not specified." << endl
         << "Please inspect the command line help with -h or --help." << endl;
    return EXIT_FAILURE;
  } catch (po::validation_error &e) {
    cout << "Error while parsing command line options:" << endl
         << "Validation of option " << e.get_option_name() << " failed." << endl
         << "Please inspect the command line help with -h or --help." << endl;
    return EXIT_FAILURE;
  } catch (po::error &e) {
    cout << "Error while parsing command line options:" << endl
         << "No further information as to the nature of this error is "
            "available, please check your command line arguments." << endl
         << "Please inspect the command line help with -h or --help." << endl;
    return EXIT_FAILURE;
  }

  if (not vm.count("seed"))
    seed = random_device()();

  try {
    if (paths.empty()) {
      shuffle(cin, n, seed);
    } else
      for (auto &path : paths) {
        if (boost::filesystem::exists(path)) {
          ifstream ifs(path.c_str());
          shuffle(ifs, n, seed++);
        } else
          throw Exception::File::Existence(path);
      }
  } catch (runtime_error &e) {
    cout << e.what() << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
