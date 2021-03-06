/*
 * =====================================================================================
 *
 *       Filename:  harmonization.cpp
 *
 *    Description:  Ensure that motif, data, and objective specifications are consistent
 *
 *        Created:  Sun Jan 25 2015 16:05:34 +0200
 *         Author:  Jonas Maaskola <jonas@maaskola.de>
 *
 * =====================================================================================
 */

#include "harmonization.hpp"

using namespace std;

namespace Specification {
namespace Exception {
namespace Motif {
NameNotUnique::NameNotUnique(const string &name)
    : runtime_error("Error: motif name '" + name + "' not unique.") {}
MultiplicityZero::MultiplicityZero(const string &name)
    : runtime_error("Error: multiplicity of motif '" + name + "' is zero!") {}
NameNotUniqueInObjective::NameNotUniqueInObjective(const string &name)
    : runtime_error("Error: motif name '" + name
                    + "'in objective not unique.") {}
NoSpecfication::NoSpecfication(const string &name)
    : runtime_error(
          "Error: found a motif name in the objective for which "
          "no motif specification is found: '" + name + "'.") {}
}
namespace Objective {
MultpleObjectivesWithoutNamedMotifs::MultpleObjectivesWithoutNamedMotifs()
    : runtime_error(
          "Error: multiple score specifications given that do not name "
          "motifs.") {}
NoMotif::NoMotif(const string &token)
    : runtime_error("Error: motif specification '" + token
                    + "' is not associated with any motif.") {}
}
}
}
