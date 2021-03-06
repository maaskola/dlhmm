/* =====================================================================================
 * Copyright (c) 2012, Jonas Maaskola
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
 *       Filename:  results.hpp
 *
 *    Description:  Data structures to store Plasma results
 *
 *        Created:  Thu May 31 06:47:48 2012 +0200
 *
 *         Author:  Jonas Maaskola <jonas@maaskola.de>
 *
 * =====================================================================================
 */

#ifndef RESULTS_HPP
#define RESULTS_HPP

#include <string>
#include "../matrix.hpp"
#include "options.hpp"

namespace Seeding {
struct Result : public Objective {
  std::string motif;
  double score;
  double log_p;
  count_vector_t counts;
  Result(const Objective &objective);
};
using Results = std::vector<Result>;
}

#endif /* ----- #ifndef RESULTS_HPP  ----- */
