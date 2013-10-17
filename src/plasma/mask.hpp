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
 *       Filename:  mask.hpp
 *
 *    Description:  Routines to mask sequences that have motif occurrences
 *
 *        Created:  Thu May 31 06:47:48 2012 +0200
 *
 *         Author:  Jonas Maaskola (JM), jonas@maaskola.de
 *
 * =====================================================================================
 */

#ifndef  MASK_HPP
#define  MASK_HPP

#include "options.hpp"

namespace Plasma {
  void remove_seqs_with_motif(const std::string &motif, DataSet &data_set, const options_t &options);
  bool mask_motif_occurrences(const std::string &motif, std::string &seq, const options_t &options, char mask_symbol='n');
  void mask_motif_occurrences(const std::string &motif, DataSet &data_set, const options_t &options);
  void mask_motif_occurrences(const std::string &motif, DataSeries &data_series, const options_t &options);
  void remove_seqs_with_motif(const std::string &motif, DataSeries &data_series, const options_t &options);
  void apply_mask(DataCollection &d, const std::string &motif, const options_t &options);
};

#endif   /* ----- #ifndef MASK_HPP  ----- */
