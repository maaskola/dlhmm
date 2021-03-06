#include <iomanip>
#include <boost/algorithm/string.hpp>
#include "aux.hpp"

using namespace std;

string reverse_and_complement(const string &s) {
  string t;
  for (string::const_reverse_iterator iter = s.rbegin(); iter != s.rend();
       iter++)
    switch (*iter) {
      case 'a':
        t += "t";
        break;
      case 'c':
        t += "g";
        break;
      case 'g':
        t += "c";
        break;
      case 't':
      case 'u':
        t += "a";
        break;
      case 'n':
        t += "n";
        break;
      case 'A':
        t += "T";
        break;
      case 'C':
        t += "G";
        break;
      case 'G':
        t += "C";
        break;
      case 'T':
      case 'U':
        t += "A";
        break;
      case 'N':
        t += "N";
        break;
      default:
        throw Exception::NucleicAcids::InvalidNucleotideCode(*iter);
    }
  return t;
}

double norml0(const matrix_t &a) {
  double d = 0;
  for (size_t i = 0; i < a.size1(); i++)
    for (size_t j = 0; j < a.size2(); j++)
      d += a(i, j);
  return d;
}

double norml1(const matrix_t &a) {
  double d = 0;
  for (size_t i = 0; i < a.size1(); i++)
    for (size_t j = 0; j < a.size2(); j++)
      d += fabs(a(i, j));
  return d;
}

double norml2(const matrix_t &a) {
  double d = 0;
  for (size_t i = 0; i < a.size1(); i++)
    for (size_t j = 0; j < a.size2(); j++) {
      double x = fabs(a(i, j));
      d += x * x;
    }
  d = sqrt(d);
  return d;
}

double normlp(const matrix_t &a, double p) {
  double d = 0;
  for (size_t i = 0; i < a.size1(); i++)
    for (size_t j = 0; j < a.size2(); j++) {
      double x = fabs(a(i, j));
      d += pow(x, p);
    }
  d = pow(d, 1 / p);
  return d;
}

double norml1(const matrix_t &a, const matrix_t &b) {
  double d = 0;
  for (size_t i = 0; i < a.size1(); i++)
    for (size_t j = 0; j < a.size2(); j++)
      d += fabs(a(i, j) - b(i, j));
  return d;
}

/*
double norml2(const matrix_t &a, const matrix_t &b)
{
  double d = 0;
  for(size_t i = 0; i < a.size1(); i++)
    for(size_t j = 0; j < a.size2(); j++) {
      double x = a(i,j);
      d += x * x;
    }
  for(size_t i = 0; i < b.size1(); i++)
    for(size_t j = 0; j < b.size2(); j++) {
      double x = b(i,j);
      d += x * x;
    }
  d = sqrt(d);
  return d;
}
*/

double normlp(const matrix_t &a, const matrix_t &b, double p) {
  double d = 0;
  for (size_t i = 0; i < a.size1(); i++)
    for (size_t j = 0; j < a.size2(); j++)
      d += pow(a(i, j), p);
  for (size_t i = 0; i < b.size1(); i++)
    for (size_t j = 0; j < b.size2(); j++)
      d += pow(b(i, j), p);
  d = pow(d, 1 / p);
  return d;
}

vector<size_t> shift_positions_left(const vector<size_t> original_inserts,
                                    size_t shift) {
  vector<size_t> inserts;
  for (auto p : original_inserts)
    if (p >= shift)
      inserts.push_back(p - shift);
  return inserts;
}

vector<size_t> shift_positions_right(const vector<size_t> original_inserts,
                                     size_t shift, size_t maxp) {
  vector<size_t> inserts;
  for (auto p : original_inserts)
    if (p < maxp - shift)
      inserts.push_back(p + shift);
  return inserts;
}

vector<size_t> parse_list(const string &s) {
  size_t pos = s.find_first_not_of("0123456789,-");
  if (pos != string::npos)
    throw Exception::NumberList::InvalidCharacter(s, pos);

  vector<string> strs;
  boost::split(strs, s, boost::is_any_of(","));

  vector<size_t> v;

  for (auto t : strs) {
    if (t.size() > 0) {
      vector<string> s2;
      boost::split(s2, t, boost::is_any_of("-"));
      size_t i = 0, j = 0;
      switch (s2.size()) {
        case 1:
          i = atoi(s2[0].c_str());
          v.push_back(i);
          break;
        case 2:
          i = atoi(s2[0].c_str());
          j = atoi(s2[1].c_str());
          for (size_t k = i; k <= j; k++)
            v.push_back(k);
          break;
        default:
          throw Exception::NumberList::MultipleRanges(t);
      }
    }
  }
  return v;
}

istream &safeGetline(istream &is, string &t) {
  t.clear();

  // The characters in the stream are read one-by-one using a streambuf.
  // That is faster than reading them one-by-one using the istream.
  // Code that uses streambuf this way must be guarded by a sentry object.
  // The sentry object performs various tasks,
  // such as thread synchronization and updating the stream state.

  istream::sentry se(is, true);
  streambuf *sb = is.rdbuf();

  for (;;) {
    int c = sb->sbumpc();
    switch (c) {
      case '\r':
        c = sb->sgetc();
        if (c == '\n')
          sb->sbumpc();
        return is;
      case '\n':
      case EOF:
        return is;
      default:
        t += (char)c;
    }
  }
}

string string_tolower(const string &str) {
  string temp = str;
  range_tolower(temp.begin(), temp.end());
  return temp;
}

string string_toupper(const string &str) {
  string temp = str;
  range_toupper(temp.begin(), temp.end());
  return temp;
}

string break_line(const string &s, size_t line_length) {
  string x = s;
  string y = "";
  while (x.size() > line_length) {
    size_t pos = x.find_last_of(" ", line_length);
    y += x.substr(0, pos) + "\n";
    if (pos < x.size())
      x = x.substr(pos + 1);
    else
      x = "";
  }
  y += x;
  return y;
}

string limit_line_length(const string &x, size_t line_length) {
  string y = "";
  size_t pos = 0;
  while (pos != string::npos) {
    size_t next_pos = x.find("\n", pos);
    y += ((pos != 0) ? "\n" : "")
         + break_line(x.substr(pos, next_pos - pos), line_length);
    if (next_pos != string::npos)
      pos = next_pos + 1;
    else
      pos = string::npos;
  }
  return y;
}

vector<string> tokenize(const string &s, const string &delim) {
  vector<string> strs;
  boost::split(strs, s, boost::is_any_of(delim));
  return strs;
}

string to_pretty_string(double x, int width, int num_digits) {
  stringstream ss;
  if (num_digits >= 0) {
    ss.setf(ios_base::fixed, ios_base::floatfield);
    ss.precision(num_digits);
  } else
    ss.precision(10);
  if (width >= 0)
    ss << setw(width);
  ss << right << x;
  return ss.str();
}

/** Format a time duration with a time unit separated by a blank
 * Expects microseconds as argument
 * if x > 1e6 µs it will print in seconds
 * if x > 1e3 µs it will print in milliseconds
 * if x > 1e3 µs it will print in microseconds
 */
string time_to_pretty_string(double x, int width, int num_digits) {
  double y = x;
  size_t scale = 1;
  if (y > 1e6)
    scale = 1000000;
  else if (y > 1e3)
    scale = 1000;
  y /= scale;
  string s = to_pretty_string(y, width, num_digits);
  switch (scale) {
    case 1:
      s += " µs";
      break;
    case 1000:
      s += " ms";
      break;
    case 1000000:
      s += " s";
      break;
    default:
      // can't happen
      break;
  }
  return s;
}

namespace Exception {
namespace NumberList {
InvalidCharacter::InvalidCharacter(const string &spec, size_t pos)
    : runtime_error(string("Error: found invalid character '") + spec[pos]
          + "' in number list specification '" + spec + "'.\n"
          + "Please note that the format for the list specification only "
            "allows " + "digits, '-', and ','.") {}
MultipleRanges::MultipleRanges(const string &group)
    : runtime_error(
          "List format error: only one '-' is allowed in any group.\n"
          "The offending group is '" + group + "'.") {}
}
namespace NucleicAcids {
InvalidNucleotideCode::InvalidNucleotideCode(char nucl)
    : runtime_error(string("Error: found invalid nucleotide code '") + nucl
                    + "'.") {}
}
}
