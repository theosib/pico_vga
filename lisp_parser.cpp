#include "lisp.hpp"


std::ostream& Token::print_octal(std::ostream& os, int val, int len)
{
    for (int i=len-1; i>=0; i--) {
        char c = ((val >> (i*3)) & 7) + '0';
        os << c;
    }
    return os;
}

std::ostream& Token::print_string(std::ostream& os, const char *s, int len)
{
    for (int i=0; i<len; i++) {
        char c = s[i];
        if (c < 32) {
            os << '\\';
            print_octal(os, c, 3);
        } else {
            os << c;
        }
    }
    return os;
}

std::ostream& LispInterpreter::print_item(std::ostream& os, TokenPtr t, ContextPtr context)
{
    if (t->quote) os << '\'';
    switch (t->type) {
    case Token::BOOL:
        os << (t->ival ? "true" : "false");
        break;
    case Token::NONE:
        os << "nil";
        break;
    case Token::LIST:
        os << '{';
        print_list(os, t->list);
        os << '}';
        break;
    case Token::INFIX:
        os << '(';
        print_list(os, t->list);
        os << ')';
        break;
    case Token::INT:
        os << t->ival;
        break;
    case Token::STR:
        os << '\"';
        Token::print_string(os, t->sym->p, t->sym->len);
        os << '\"';
        break;
    case Token::FLOAT:
        os << t->fval;
        break;
    case Token::FUNC:
        // XXX Chase down context and print full path
        os << "func(" << t->context->get_full_path(t->sym) << ")";
        break;
    case Token::OPER:
    case Token::SYM:
        os << t->sym;
        break;
    case Token::CLASS:
        // XXX print full name
        os << "class(" << t->context->get_full_path(t->sym) << ")";
        break;
    case Token::OBJECT:
        // XXX print full name
        os << "object(" << t->context->get_full_path(t->sym) << ")";
        break;
    }
    
    return os;
}

std::ostream& LispInterpreter::print_list(std::ostream& os, TokenPtr t, ContextPtr context)
{
    bool first = true;
    while (t) {
        if (!first) os << ' ';
        first = false;
        print_item(os, t, context);
        t = t->next;
    }
    
    return os;
}

int Token::parse_octal(const char *src, int len)
{
    int val = 0;
    while (len) {
        len--;
        int c = *src++ - '0';
        val = (val * 8) + c;
    }
    return val;
}

int Token::parse_decimal(const char *src, int len)
{
    int val = 0;
    bool neg = false;
    if (len && (*src == '-' || *src == '+')) {
        neg = (*src == '-');
        len--;
        src++;
    }
    while (len) {
        len--;
        int c = *src++ - '0';
        val = (val * 10) + c;
    }
    if (neg) val = -val;
    return val;
}

int Token::parse_hex(const char *src, int len)
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

static char translate_escape(char e)
{
    for (int i=0; i<num_escapes; i++) {
        if (e == escapes[i][0]) return escapes[i][1];
    }
    return e;
}

int Token::parse_string(const char *src, int len_in, char *dst)
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
            len_out++;
        }
    }
    
    return len_out;
}

float Token::parse_float(const char *str, int length)
{
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

TokenPtr Token::parse_number(Parsing& p)
{
    TokenPtr t;
    char c;
    int len;

    p.set_mark();
    char first = p.peek();
    
    bool is_float = false;
    int base = 10;
    if (first == '0') {
        p.skip();
        c = p.peek();
        if (c == 'x') {
            base = 16;
            p.skip();
        } else if (c == '.') {
            p.rewind();
        } else if (c >= '0' && c <= '7') {
            base = 8;
            p.rewind();
        } else {
            p.rewind();
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
            t = std::make_shared<Token>();
            t->type = Token::INT;
            t->ival = parse_octal(p.get_mark() + 1, p.mark_len());
            return t;
        } else {
            return Token::zero;
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
            t = std::make_shared<Token>();
            t->type = Token::INT;
            t->ival = parse_hex(p.get_mark() + 2, p.mark_len());
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
            t = std::make_shared<Token>();
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
    
    return 0;
}

SymbolPtr LispInterpreter::parse_symbol(const char *str, int len)
{
    char ch;
    Parsing p(str, len);
    SymbolPtr s, c;
    while (p.more()) {
        p.set_mark();
        while (p.more() && p.peek() != '.') p.skip();
        SymbolPtr n = interns.find(p.get_mark(), p.mark_len())->wrap();
        if (c) {
            c->next = n;
            c = n;
        } else {
            s = n;
            c = n;
        }
        p.skip();
    }
    return s;
}

TokenPtr LispInterpreter::parse_token(Parsing& p)
{
    char *s;
    bool quote = false;
    char c;
    int len;
    TokenPtr t;
    p.skip_space();
    
    char first = p.peek();
    switch (first) {
    case 0:
        return 0;
    case ')':
    case '}':
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
        
        t = std::make_shared<Token>();
        t->type = Token::STR;
        len = p.mark_len();
        s = new char[len];
        len = Token::parse_string(p.get_mark(), len, s);
        t->sym = interns.find(s, len);
        delete [] s;
        t->quote = quote;
        p.skip();
        return t;
        
    case '{':
        p.skip();
        t = std::make_shared<Token>();
        t->type = Token::LIST;
        t->list = parse_string(p);
        t->quote = quote;
        return t;
        
    case '(':
        p.skip();
        t = std::make_shared<Token>();
        t->type = Token::INFIX;
        t->list = parse_string(p);
        t->quote = quote;
        if (quote) {
            return t;
        } else {
            return transform_infix(t);
        }
    }
    
    if (isdigit(first) || first == '.' || first == '-' || first == '+') {
        t = Token::parse_number(p);
        if (t) {
            t->quote = quote;
            return t;
        }
        
        while ((c = p.peek())) {
            if (isspace(c) || c == ')' || c == '}' || c == '(' || c == '{') {
                break;
            } else {
                p.skip();
            }
        }
        
        t = std::make_shared<Token>();
        t->type = Token::SYM;
        t->quote = quote;
        t->sym = parse_symbol(p.get_mark(), p.mark_len());
        return t;
    }
    
    p.set_mark();
    while ((c = p.peek())) {
        if (isspace(c) || c == ')' || c == '}' || c == '(' || c == '{') {
            break;
        } else {
            p.skip();
        }
    }
    
    t = std::make_shared<Token>();
    t->type = Token::SYM;
    t->quote = quote;
    t->sym = parse_symbol(p.get_mark(), p.mark_len());
    return t;
}


TokenPtr LispInterpreter::parse_string(Parsing& p)
{
    TokenPtr l;
    TokenPtr c;
    for (;;) {
        TokenPtr n = parse_token(p);
        if (!n) break;
        if (!l) {
            l = n;
            c = n;
        } else {
            c->next = n;
            c = n;
        }
    }
    
    return l;
}
