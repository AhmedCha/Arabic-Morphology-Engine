#ifndef LANGUAGE_H
#define LANGUAGE_H

#include <string>

enum AppLang { EN, FR, AR };
inline AppLang currentLang = EN;

inline std::string Tr(const std::string& en, const std::string& fr, const std::string& ar) {
    if (currentLang == FR) return fr;
    if (currentLang == AR) return ar;
    return en;
}

#endif
