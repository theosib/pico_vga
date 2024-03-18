
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

struct Token;
typedef Token *(*built_in_f)(Token *);

struct Token {
    enum {
        NONE,
        LIST,
        INT,
        STR,
        FLOAT,
        CHAR,
        OPER,
        SYM,
        ARR
    };
    
    uint8_t type;
    uint8_t quote;
    uint16_t len;
    union {
        Token *list;  // also array
        int32_t ival; // also symbol
        float fval;
        char *str;
        built_in_f oper;
    };
    
    Token *next;
    
    Token() {
        next = 0;
        list = 0;
        type = 0;
        len = 0;
        quote = 0;
    }
    
    ~Token() {
        switch (type) {
        case LIST:
            delete list;
            break;
        case STR:
            delete [] str;
            break;
        case ARR:
            delete [] list;
            break;
        }
        
        delete next;
    }
};

void print_string(const char *s, int len)
{
    for (int i=0; i<len; i++) {
        char c = s[i];
        if (c < 32) {
            printf("\\%03o", c);
        } else {
            printf("%c", c);
        }
    }
}

void print_list(Token *t)
{
    bool first = true;
    while (t) {
        if (!first) printf(" ");
        first = false;
        
        if (t->quote) printf("\'");
        switch (t->type) {
        case Token::NONE:
            printf("None");
            break;
        case Token::LIST:
            printf("(");
            print_list(t->list);
            printf(")");
            break;
        case Token::INT:
            printf("%d", t->ival);
            break;
        case Token::STR:
            print_string(t->str, t->len);
            break;
        case Token::FLOAT:
            printf("%f", t->fval);
            break;
        case Token::SYM:
            printf("S%d", t->ival);
            break;
        }
        
        t = t->next;
    }
}

struct Parsing {
    const char *p = 0, *q = 0;
    int len = 0;
    char peek() { 
        if (len < 1) return 0;
        return *p;
    }
    void skip(int n=1) {
        if (len > n) {
            len -= n;
            p += n;
        } else {
            p += len;
            len = 0;
        }
    }
    char pop() { 
        if (len < 1) return 0;
        char val = *p++;
        len--;
        return val;
    }
    bool more() {
        return len>0;
    }
    void skip_space() {
        while (more() && isspace(*p)) { p++; len--; }
    }
    void set_mark() {
        q = p;
    }
    const char *get_mark() {
        return q;
    }
    int mark_len() {
        return p - q;
    }
    
    Parsing() {}
    Parsing(const char *ptr, int l) : p(ptr), len(l) {}
};

int intern_symbol(const char *src, int len)
{
    return 0;
}

int parse_octal(const char *src, int len)
{
    int val = 0;
    while (len) {
        len--;
        int c = *src++ - '0';
        val = (val * 8) + c;
    }
    return val;
}

int parse_decimal(const char *src, int len)
{
    int val = 0;
    while (len) {
        len--;
        int c = *src++ - '0';
        val = (val * 10) + c;
    }
    return val;
}

int parse_hex(const char *src, int len)
{
    int val = 0;
    while (len) {
        len--;
        int c = *src++;
        if (c>='a' && c<='f') {
            c = c - 'a' + 10;
        } else if (c>='A' && c<='F') {
            c = c - 'A' + 10;
        } else {
            c -= '0';
        }
        val = (val * 16) + c;
    }
    return val;
}


static char escapes[][2] = {
    'r', 13,
    'a', 7,
    'b', 8,
    'e', 27,
    'n', 10,
    't', 9,
};
constexpr int num_escapes = sizeof(escapes) / sizeof(escapes[0]);

char translate_escape(char e)
{
    for (int i=0; i<num_escapes; i++) {
        if (e == escapes[i][0]) return escapes[i][1];
    }
    return e;
}

int parse_string(const char *src, int len_in, char *dst)
{
    Parsing p(src, len_in);
    char val;
    int len_out = 0;
    char c;
    
    while ((c = p.pop())) {
        if (c == '\\') {
            c = p.peek();
            if (c == 'x') {
                p.skip();
                p.set_mark();
                for (int i=0; i<2; i++) {
                    if (isxdigit(p.peek())) {
                        p.skip();
                    } else break;
                }
                val = parse_hex(p.get_mark(), p.mark_len());
            } else if (isdigit(c)) {
                p.set_mark();
                p.skip();
                for (int i=0; i<2; i++) {
                    if (isdigit(p.peek())) {
                        p.skip();
                    } else break;
                }
                val = parse_octal(p.get_mark(), p.mark_len());
            } else {
                val = translate_escape(c);
                p.skip();
            }
            *dst++ = val;
            len_out++;
        } else {
            *dst++ = c;
        }
    }
    
    return len_out;
}




float parse_float(const char *str, int length) {
    float result = 0.0f;
    bool negative = false;
    bool decimalPointEncountered = false;
    int decimals = 0;
    bool exponentEncountered = false;
    int exponent = 0;
    bool exponent_negative = false;

    for (int i = 0; i < length; ++i) {
        char c = str[i];

        if (c == '-' && i == 0) {
            negative = true;
        } else if (c == '+' && i == 0) {
            negative = false;
        } else if (isdigit(c)) {
            if (exponentEncountered) {
                exponent = exponent * 10 + (c - '0');
            } else if (decimalPointEncountered) {
                result = result * 10 + (c - '0');
                decimals++;
            } else {
                result = result * 10 + (c - '0');
            }
        } else if ((c == 'e' || c == 'E') && !exponentEncountered) {
            exponentEncountered = true;
        } else if ((c == '-' || c == '+') && exponentEncountered && (str[i-1] == 'e' || str[i-1] == 'E')) {
            if (c == '-') {
                exponent_negative = true;
            } else {
                exponent_negative = false;
            }
        } else if (c == '.' && !decimalPointEncountered && !exponentEncountered) {
            decimalPointEncountered = true;
        } else {
            break;
        }
    }
    
    if (exponentEncountered) {
        if (exponent_negative) exponent = -exponent;
        if (decimalPointEncountered) exponent -= decimals;
        result *= pow(10, exponent);
    } else if (decimalPointEncountered) {
        result *= pow(10, -decimals);
    }
    
    if (negative) result = -result;
    return result;
}


Token *parse_list(Parsing& p);

Token *parse_token(Parsing& p)
{
    char c;
    bool quote = false;
    int len;
    Token *t;
    p.skip_space();
    
    char first = p.peek();
    switch (first) {
    case 0:
        return 0;
    case ')':
        p.pop();
        return 0;
    case '\'':
        p.pop();
        quote = true;
        p.skip_space();
        first = p.peek();
        break;
    }
    
    switch (first) {
    case '\"':
        p.pop();
        p.set_mark();
        while ((c = p.peek())) {
            if (c == '\\') {
                p.skip(2);
            } else if (c == '\"') {
                break;
            } else {
                p.skip();
            }
        }
        
        t = new Token();
        t->type = Token::STR;
        len = p.mark_len();
        t->str = new char[len];
        t->len = parse_string(p.get_mark(), len, t->str);
        t->quote = quote;
        p.skip();
        return t;
        
    case '(':
        // Somehow capture list length
        p.skip();
        t = new Token();
        t->type = Token::LIST;
        t->list = parse_list(p);
        t->quote = quote;
        return t;
    }
    
    if (isdigit(first) || first == '.' || first == '-' || first == '+') {
        p.set_mark();
        bool is_float = false;
        int base = 10;
        if (first == '0') {
            base = 8;
            p.skip();
            if (p.peek() == 'x') {
                base = 16;
                p.skip();
            }
        }        
        
        switch (base) {
        case 8:
            len = 0;
            while ((c = p.peek())) {
                if (c>='0' && c<='7') {
                    p.skip();
                    len++;
                } else break;
            }
            if (len > 0) {
                t = new Token();
                t->quote = quote;
                t->type = Token::INT;
                t->ival = parse_octal(p.get_mark() + 1, p.mark_len());
                return t;
            }
            break;
            
        case 16:
            len = 0;
            while ((c = p.peek())) {
                if (isxdigit(c)) {
                    p.skip();
                    len++;
                } else break;
            }
            if (len > 0) {
                t = new Token();
                t->quote = quote;
                t->type = Token::INT;
                t->ival = parse_octal(p.get_mark() + 2, p.mark_len());
                return t;
            }
            break;
            
        default:
            {
                c = p.peek();
                if (c == '-' || c == '+') p.skip();
            }
            len = 0;
            while ((c = p.peek())) {
                if (c == '.' || c == 'e' || c == 'E') {
                    is_float = true;
                    p.skip();
                    len++;
                } else if (isdigit(c)) {
                    p.skip();
                    len++;
                } else break;
            }
            if (len > 0) {
                t = new Token();
                t->quote = quote;
                if (is_float) {
                    t->type = Token::FLOAT;
                    t->fval = parse_float(p.get_mark(), p.mark_len());
                } else {
                    t->type = Token::INT;
                    t->ival = parse_decimal(p.get_mark(), p.mark_len());
                }
                return t;
            }
            break;
        }
        
        t = new Token();
        t->type = Token::SYM;
        t->quote = quote;
        t->ival = intern_symbol(p.get_mark(), p.mark_len());
        return t;
    }
    
    p.set_mark();
    while ((c = p.peek())) {
        if (isspace(c) || c == ')') {
            break;
        } else {
            p.skip();
        }
    }
    
    t = new Token();
    t->type = Token::SYM;
    t->quote = quote;
    t->ival = intern_symbol(p.get_mark(), p.mark_len());
    return t;
}


Token *parse_list(Parsing& p)
{
    Token *l = 0;
    Token *c = 0;
    for (;;) {
        Token *n = parse_token(p);
        if (!n) return l;
        if (!l) {
            l = n;
            c = n;
        } else {
            c->next = n;
            c = n;
        }
    }
    
    // XXX unstrap first level of list if there's only one element, and it's a list
    return l;
}




int main() {
    // const char *floatStr = "3.14e2";
    // int length = strlen(floatStr);
    // float value = parse_float(floatStr, length);
    //
    // printf("Parsed value: %f\n", value);
    
    const char *sample = "('(x y) 4 '3.14 3.14e3 \"string\033xxx\") 123";
    int len = strlen(sample);
    Parsing pa(sample, len);
    Token *p = parse_list(pa);
    print_list(p);
    return 0;
}
