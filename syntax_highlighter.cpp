#include "syntax_highlighter.h"
#include <algorithm>
#include <filesystem>
#include <cctype>

SyntaxHighlighter::SyntaxHighlighter() : current_lang(LANG_PLAIN) {}

void SyntaxHighlighter::initColors() {
    // Inizializzazione coppie di colori con sfondo trasparente (-1)
    init_pair(COLOR_KEYWORD, COLOR_CYAN, -1);
    init_pair(COLOR_FUNCTION, COLOR_YELLOW, -1);
    init_pair(COLOR_STRING, COLOR_GREEN, -1);
    init_pair(COLOR_COMMENT, COLOR_RED, -1);
    init_pair(COLOR_NUMBER, COLOR_MAGENTA, -1);
}

void SyntaxHighlighter::setLanguage(Language lang) {
    current_lang = lang;
}

Language SyntaxHighlighter::detectLanguage(const std::string& filename) {
    std::string ext = std::filesystem::path(filename).extension().string();
    if (ext == ".cpp" || ext == ".hpp" || ext == ".h" || ext == ".c") return LANG_CPP;
    if (ext == ".py") return LANG_PYTHON;
    if (ext == ".js") return LANG_JAVASCRIPT;
    return LANG_PLAIN;
}

bool SyntaxHighlighter::isKeyword(const std::string& word) {
    const std::vector<std::string>* keywords;
    if (current_lang == LANG_CPP) keywords = &cpp_keywords;
    else if (current_lang == LANG_PYTHON) keywords = &python_keywords;
    else if (current_lang == LANG_JAVASCRIPT) keywords = &js_keywords;
    else return false;

    return std::find(keywords->begin(), keywords->end(), word) != keywords->end();
}

bool SyntaxHighlighter::isFunctionCall(const std::string& line, size_t pos) {
    while (pos < line.length() && std::isspace(line[pos])) pos++;
    return (pos < line.length() && line[pos] == '(');
}

std::vector<Token> SyntaxHighlighter::tokenizeLine(const std::string& line) {
    std::vector<Token> tokens;
    if (line.empty()) return tokens;

    size_t i = 0;
    while (i < line.length()) {
        // Commenti a riga singola
        if ((current_lang == LANG_CPP || current_lang == LANG_JAVASCRIPT) && i + 1 < line.length() && line[i] == '/' && line[i+1] == '/') {
            tokens.push_back({line.substr(i), TOKEN_COMMENT, (int)i, (int)line.length()});
            break;
        }
        if (current_lang == LANG_PYTHON && line[i] == '#') {
            tokens.push_back({line.substr(i), TOKEN_COMMENT, (int)i, (int)line.length()});
            break;
        }

        // Stringhe
        if (line[i] == '"' || line[i] == '\'') {
            char quote = line[i];
            size_t start = i++;
            while (i < line.length() && line[i] != quote) {
                if (line[i] == '\\' && i + 1 < line.length()) i++;
                i++;
            }
            if (i < line.length()) i++;
            tokens.push_back({line.substr(start, i - start), TOKEN_STRING, (int)start, (int)i});
            continue;
        }

        // Numeri
        if (std::isdigit(line[i])) {
            size_t start = i;
            while (i < line.length() && (std::isdigit(line[i]) || line[i] == '.')) i++;
            tokens.push_back({line.substr(start, i - start), TOKEN_NUMBER, (int)start, (int)i});
            continue;
        }

        // Parole chiave e Funzioni
        if (std::isalpha(line[i]) || line[i] == '_') {
            size_t start = i;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_')) i++;
            std::string word = line.substr(start, i - start);
            
            TokenType type = TOKEN_NORMAL;
            if (isKeyword(word)) type = TOKEN_KEYWORD;
            else if (isFunctionCall(line, i)) type = TOKEN_FUNCTION;
            
            tokens.push_back({word, type, (int)start, (int)i});
            continue;
        }

        // Caratteri normali / Operatori
        tokens.push_back({std::string(1, line[i]), TOKEN_NORMAL, (int)i, (int)i + 1});
        i++;
    }
    return tokens;
}

void SyntaxHighlighter::renderLineWithHighlight(int screen_row, const std::string& line, int vcol_offset, int max_cols) {
    auto tokens = tokenizeLine(line);
    move(screen_row, 0);

    int current_vcol = 0; // Colonna virtuale (visiva)

    for (const auto& token : tokens) {
        int pair = 0;
        switch (token.type) {
            case TOKEN_KEYWORD:  pair = COLOR_KEYWORD; break;
            case TOKEN_FUNCTION: pair = COLOR_FUNCTION; break;
            case TOKEN_STRING:   pair = COLOR_STRING; break;
            case TOKEN_COMMENT:  pair = COLOR_COMMENT; break;
            case TOKEN_NUMBER:   pair = COLOR_NUMBER; break;
            default:             pair = 0; break;
        }

        if (pair > 0) attron(COLOR_PAIR(pair));

        for (char c : token.text) {
            // Calcolo larghezza carattere (Tab impostato a 2 spazi)
            int char_width = (c == '\t') ? (2 - (current_vcol % 2)) : 1;

            // Renderizza solo se visibile nella finestra (viewport)
            if (current_vcol + char_width > vcol_offset && current_vcol < vcol_offset + max_cols) {
                if (c == '\t') {
                    for (int s = 0; s < char_width; s++) {
                        int vis_pos = current_vcol + s;
                        if (vis_pos >= vcol_offset && vis_pos < vcol_offset + max_cols) {
                            addch(' ');
                        }
                    }
                } else {
                    if (current_vcol >= vcol_offset) {
                        addch(c);
                    }
                }
            }
            current_vcol += char_width;
        }

        if (pair > 0) attroff(COLOR_PAIR(pair));
    }
}
