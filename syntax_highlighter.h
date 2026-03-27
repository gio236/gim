#ifndef SYNTAX_HIGHLIGHTER_H
#define SYNTAX_HIGHLIGHTER_H

#include <string>
#include <vector>
#include <ncurses.h>

enum TokenType {
    TOKEN_NORMAL,
    TOKEN_KEYWORD,
    TOKEN_FUNCTION,
    TOKEN_STRING,
    TOKEN_COMMENT,
    TOKEN_NUMBER,
    TOKEN_OPERATOR
};

struct Token {
    std::string text;
    TokenType type;
    int start;
    int end;
};

enum Language {
    LANG_CPP,
    LANG_PYTHON,
    LANG_JAVASCRIPT,
    LANG_PLAIN
};

class SyntaxHighlighter {
private:
    Language current_lang;
    
    // Color pairs (starting from 2 since 1 is used for status bar)
    static constexpr int COLOR_KEYWORD = 2;
    static constexpr int COLOR_FUNCTION = 3;
    static constexpr int COLOR_STRING = 4;
    static constexpr int COLOR_COMMENT = 5;
    static constexpr int COLOR_NUMBER = 6;

    std::vector<std::string> cpp_keywords = {
        "int", "float", "double", "char", "bool", "void", "class", "struct",
        "if", "else", "while", "for", "return", "const", "static", "public",
        "private", "protected", "virtual", "namespace", "using", "template",
        "auto", "nullptr", "true", "false", "new", "delete", "this", "include"
    };
    
    std::vector<std::string> python_keywords = {
        "def", "class", "if", "elif", "else", "while", "for", "return",
        "import", "from", "as", "try", "except", "finally", "with", "lambda",
        "True", "False", "None", "and", "or", "not", "in", "is", "pass",
        "break", "continue", "yield", "async", "await"
    };
    
    std::vector<std::string> js_keywords = {
        "function", "const", "let", "var", "if", "else", "while", "for",
        "return", "class", "new", "this", "async", "await", "try", "catch",
        "finally", "throw", "import", "export", "default", "from", "true",
        "false", "null", "undefined", "typeof", "instanceof"
    };

    bool isKeyword(const std::string& word);
    bool isFunctionCall(const std::string& line, size_t pos);

public:
    SyntaxHighlighter();
    void initColors();
    void setLanguage(Language lang);
    Language detectLanguage(const std::string& filename);
    std::vector<Token> tokenizeLine(const std::string& line);
    void renderLineWithHighlight(int screen_row, const std::string& line, int vcol_offset, int max_cols);
};

#endif
