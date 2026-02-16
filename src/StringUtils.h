#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <string>
#include <vector>
#include <string_view>

using namespace std;

class StringUtils {
  public:
    // Strip invisible carriage returns, tabs, and accidental spaces
    static string sanitize(const string& str) {
      string out;
      for (char c : str) {
        if (c != '\r' && c != '\n' && c != '\t' && c != ' ') {
          out += c;
        }
      }
      return out;
    }

    // Splits a UTF-8 string into individual characters safely
    static vector<string_view> splitUTF8(string_view str) {
      vector<string_view> chars;
      for (size_t i = 0; i < str.length();) {
        unsigned char c = str[i];
        int len = 1;

        if ((c & 0xE0) == 0xC0) len = 2;
        else if ((c & 0xF0) == 0xE0) len = 3;
        else if ((c & 0xF8) == 0xF0) len = 4;

        string_view currentChar = str.substr(i, len);
        chars.push_back(currentChar);

        i += len;
      }
      return chars;
    }

    // Returns the true character count of a UTF-8 string
    static int utf8_length(string_view str) {
      return splitUTF8(str).size();
    }

    // Normalize Alif variations to bare Alif
    static string_view normalizeAlif(string_view c) {
      if (c == "\u0623" || // أ
          c == "\u0625" || // إ
          c == "\u0622" || // آ
          c == "\u0621" || // ء
          c == "\u0624" || // ؤ
          c == "\u0626" || // ئ
          c == "\u0671")   // ٱ
      {
          return "\u0623"; // أ
      }
      return c;
    }
};

#endif
