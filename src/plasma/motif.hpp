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
 *       Filename:  motif.hpp
 *
 *    Description:  Data structure for regular expression type motifs
 *
 *        Created:  Thu May 31 06:47:48 2012 +0200
 *
 *         Author:  Jonas Maaskola (JM), jonas@maaskola.de
 *
 * =====================================================================================
 */

#ifndef MOTIF_HPP
#define MOTIF_HPP

#include <cstdint>
#include <list>
#include <string>
#include "data.hpp"
#include "stats.hpp"
#include "../verbosity.hpp"

namespace Plasma {
  /** A class for IUPAC regular expression type motifs of up to length 16.
   * Uses a binary representation of size 64 bit with 4 bits per position. */
/*  struct Motif {
    // TODO write long motif class
    // TODO use SSE2
    typedef std::string storage_t;
//    static const storage_t block = 0;
    storage_t data;
    size_t length;
    Motif();
    Motif(storage_t d);
    Motif(const std::string &s);

    Motif reverse_complement() const;
    std::string to_string(storage_t x) const;
    std::string to_string() const;

    bool match(const Sequence &seq, size_t &pos, Verbosity verbosity) const;

    size_t degeneracy() const;
  };
  */
  double information_content(const std::string &motif);
  size_t motif_degeneracy(const std::string &motif);
  template <typename Func> void each_generalization(const std::string &motif, Func func) {
    // std::cout << "Generalizations of " << to_string() << std::endl;
    std::string generalization = motif;
    for(size_t i = 0; i < motif.size(); i++) {
      char current = generalization[i];
      switch(current) {
        case 'a':
          generalization[i] = 'm';
          func(generalization);
          generalization[i] = 'r';
          func(generalization);
          generalization[i] = 'w';
          func(generalization);
          break;
        case 'c':
          generalization[i] = 'm';
          func(generalization);
          generalization[i] = 's';
          func(generalization);
          generalization[i] = 'y';
          func(generalization);
          break;
        case 'g':
          generalization[i] = 'r';
          func(generalization);
          generalization[i] = 's';
          func(generalization);
          generalization[i] = 'k';
          func(generalization);
          break;
        case 't':
          generalization[i] = 'w';
          func(generalization);
          generalization[i] = 'y';
          func(generalization);
          generalization[i] = 'k';
          func(generalization);
          break;
        case 'w':
          generalization[i] = 'd';
          func(generalization);
          generalization[i] = 'h';
          func(generalization);
        case 's':
          generalization[i] = 'b';
          func(generalization);
          generalization[i] = 'v';
          func(generalization);
        case 'm':
          generalization[i] = 'h';
          func(generalization);
          generalization[i] = 'v';
          func(generalization);
        case 'k':
          generalization[i] = 'b';
          func(generalization);
          generalization[i] = 'd';
          func(generalization);
        case 'r':
          generalization[i] = 'd';
          func(generalization);
          generalization[i] = 'v';
          func(generalization);
        case 'y':
          generalization[i] = 'b';
          func(generalization);
          generalization[i] = 'h';
          func(generalization);
        case 'b':
        case 'd':
        case 'h':
        case 'v':
          generalization[i] = 'n';
          func(generalization);
        default:
          break;
      }
      generalization[i] = current;
    }
  };

  std::list<std::string> all_generalizations(const std::string &motif);
};

// std::ostream &operator<<(std::ostream &os, const Plasma::Motif &motif);

#endif   /* ----- #ifndef MOTIF_HPP ----- */
