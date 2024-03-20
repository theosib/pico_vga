
#include "lisp.hpp"
#include <sstream>



std::ostream& operator<<(std::ostream& os, const TokenPtr& s)
{
    LispInterpreter::print_item(os, s);
    return os;
}


void Context::set_global(SymbolPtr s, TokenPtr t)
{
    if (!parent) {
        set(s, t);
    } else {
        parent->set_global(s, t);
    }
}

TokenPtr Context::get(SymbolPtr s)
{
    TokenPtr t = vars.get(s);
    if (t) return t;
    if (!parent) return 0;
    return parent->get(s);
}

SymbolPtr Interns::find(const char *p, int len)
{
    int free_index = -1;
    for (int i=0; i<syms.size(); i++) {
        SymbolPtr s = syms[i].lock();
        if (!s) {
            free_index = i;
        } else {
            if (len == s->len && 0==memcmp(p, s->p, len)) return s;
        }
    }
    if (free_index < 0) {
        free_index = syms.size();
        syms.resize(free_index+1);
    }
    SymbolPtr s = std::make_shared<Symbol>();
    s->len = len;
    s->p = new char[len];
    s->index = free_index;
    memcpy(s->p, p, len);
    syms[free_index] = s;
    return s;
}


static std::ostream& print_octal(std::ostream& os, int val, int len)
{
    for (int i=len-1; i>=0; i--) {
        char c = ((val >> (i*3)) & 7) + '0';
        os << c;
    }
    return os;
}

static std::ostream& print_string(std::ostream& os, const char *s, int len)
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

std::ostream& LispInterpreter::print_item(std::ostream& os, TokenPtr t)
{
    if (t->quote) os << '\'';
    switch (t->type) {
    case Token::NONE:
        os << "()";
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
        print_string(os, t->sym->p, t->sym->len);
        os << '\"';
        break;
    case Token::FLOAT:
        os << t->fval;
        break;
    case Token::FUNC:
    case Token::OPER:
    case Token::SYM:
        os << t->sym;
        break;
    }
    
    return os;
}

std::ostream& LispInterpreter::print_list(std::ostream& os, TokenPtr t)
{
    bool first = true;
    while (t) {
        if (!first) os << ' ';
        first = false;
        print_item(os, t);
        t = t->next;
    }
    
    return os;
}

static int parse_octal(const char *src, int len)
{
    int val = 0;
    while (len) {
        len--;
        int c = *src++ - '0';
        val = (val * 8) + c;
    }
    return val;
}

static int parse_decimal(const char *src, int len)
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

static int parse_hex(const char *src, int len)
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

static int parse_string(const char *src, int len_in, char *dst)
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

static float parse_float(const char *str, int length) {
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

TokenPtr parse_number(Parsing& p)
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
        len = ::parse_string(p.get_mark(), len, s);
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
        t = parse_number(p);
        if (t) {
            t->quote = quote;
            return t;
        }
        
        while ((c = p.peek())) {
            if (isspace(c) || c == ')') {
                break;
            } else {
                p.skip();
            }
        }
        
        t = std::make_shared<Token>();
        t->type = Token::SYM;
        t->quote = quote;
        t->sym = interns.find(p.get_mark(), p.mark_len());
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
    
    t = std::make_shared<Token>();
    t->type = Token::SYM;
    t->quote = quote;
    t->sym = interns.find(p.get_mark(), p.mark_len());
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


TokenPtr Token::zero = Token::make_int(0);
TokenPtr Token::one = Token::make_int(1);
TokenPtr Token::negone = Token::make_int(-1);

TokenPtr Token::make_string(LispInterpreter *interp, const char *p, int len)
{
    SymbolPtr s = interp->interns.find(p, len);
    TokenPtr l = std::make_shared<Token>();
    l->type = STR;
    l->sym = s;
    return l;
}

TokenPtr Token::as_number(TokenPtr item)
{
    if (!item) return zero;
    if (item->type == INT || item->type == FLOAT) return item;
    if (item->type == STR) {
        Parsing p(item->sym->p, item->sym->len);
        TokenPtr t = parse_number(p);
        if (t->type == INT || t->type == FLOAT) return t;
    }
    return zero;
}

TokenPtr Token::as_int(TokenPtr item)
{
    TokenPtr t = as_number(item);
    if (t->type == INT) {
        return t;
    } else {
        return make_int(t->fval);
    }
}

TokenPtr Token::as_float(TokenPtr item)
{
    TokenPtr t = as_number(item);
    if (t->type == FLOAT) {
        return t;
    } else {
        return make_float(t->ival);
    }
}

float Token::float_val(TokenPtr item)
{
    if (!item) return 0;
    if (item->type == FLOAT) return item->fval;
    if (item->type == INT) return item->ival;
    if (item->type == STR) {
        Parsing p(item->sym->p, item->sym->len);
        TokenPtr t = parse_number(p);
        if (t->type == FLOAT) return t->fval;
        if (t->type == INT) return t->ival;
    }
    return 0;
}

int Token::int_val(TokenPtr item)
{
    if (!item) return 0;
    if (item->type == FLOAT) return item->fval;
    if (item->type == INT) return item->ival;
    if (item->type == STR) {
        Parsing p(item->sym->p, item->sym->len);
        TokenPtr t = parse_number(p);
        if (t->type == FLOAT) return t->fval;
        if (t->type == INT) return t->ival;
    }
    return 0;
}

TokenPtr Token::as_string(LispInterpreter *interp, TokenPtr item) 
{
    if (!item) return interp->empty_string;
    if (item->type == STR) return item;
    std::stringstream ss;
    LispInterpreter::print_item(ss, item);
    return make_string(interp, ss.str());
}

std::string Token::string_val(TokenPtr item)
{
    if (!item) return "";
    if (item->type == STR) return std::string(item->sym->p, item->sym->len);
    std::stringstream ss;
    LispInterpreter::print_item(ss, item);
    return ss.str();
}

void LispInterpreter::addOperator(const std::string& name, built_in_f f, int precedence)
{
    SymbolPtr s = interns.find(name.data(), name.length());
    TokenPtr t = std::make_shared<Token>();
    t->type = Token::OPER;
    t->oper = f;
    t->sym = s;
    t->precedence = precedence;
    globals->set(s, t);
}

void LispInterpreter::addFunction(TokenPtr list, ContextPtr context)
{
    // Extract name and arg list
    // XXX Check to make sure there's an args list
    // XXX Check for nulls
    if (list->type != Token::SYM) return;
    TokenPtr t = std::make_shared<Token>();
    t->type = Token::FUNC;
    t->sym = list->sym;
    t->list = list->next;
    context->set(list->sym, t);
}

TokenPtr LispInterpreter::callFunction(TokenPtr list, ContextPtr parent)
{
    std::cout << "Calling func: ";
    print_list(std::cout, list);
    std::cout << std::endl;
    
    if (list->type != Token::SYM) {
        printf("Error not a token\n");
        return 0;
    }
    
    // Look up function by name
    TokenPtr func = getVariable(list, parent);
    list = list->next; // Skip over name
    
    TokenPtr args = func->list;
    std::cout << "Args: " << args << std::endl;
    
    if (args->type != Token::LIST) {
        // XXX Move this to addFunction
        printf("Function doesn't start with args list\n");
        return 0;
    }
    
    // Function body
    TokenPtr body = args->next;
    std::cout << "Body: " << body << std::endl;
    
    // New context
    ContextPtr context = parent->make_child();
    
    // args is a list of names
    args = args->list;
    while (args && list) {
        // XXX handle wrong number of arguments
        SymbolPtr arg_name = args->sym;
        if (args->quote) {
            // Final element, gets rest of args as list
            context->set(arg_name, Token::as_list(list));
            break;
        } else {
            context->set(arg_name, list);
        }
        list = list->next;
        args = args->next;
    }
    
    return evaluate_list(body, context);
}

void LispInterpreter::setVariable(TokenPtr list, ContextPtr context)
{
    if (list->type != Token::SYM) {
        printf("Not a symbol\n");
        return;
    }
    std::cout << "Set var: ";
    print_list(std::cout, list);
    std::cout << std::endl;
    context->set(list->sym, list->next);
}

TokenPtr LispInterpreter::getVariable(TokenPtr name, ContextPtr context)
{
    if (name->type != Token::SYM) return 0;
    std::cout << "Get var: " << name->sym << std::endl;
    return context->get(name->sym);
}

// Evaluate a single item
TokenPtr LispInterpreter::evaluate_item(TokenPtr item, ContextPtr context)
{
    if (!item) return 0;
    
    std::cout << "Evaluating: ";
    print_item(std::cout, item);
    std::cout << std::endl;
    
    if (item->quote) return item;    
    if (!context) context = globals;
    
    if (item->type == Token::LIST) {
        // Unquoted list is function or operator call or list of calls
        TokenPtr list = item->list;
        
        TokenPtr ret;
        while (list && list->type == Token::LIST) {
            ret = evaluate_item(list, context);
            list = list->next;
        }
        
        if (!list) return ret;
        
        if (list->type != Token::SYM) {
            std::cout << "Function call must start with symbol\n";
            return item;
        }
        
        TokenPtr func = context->get(list->sym);
        if (func->type == Token::OPER) {
            return (*func->oper)(list->next, context);
        } else if (func->type == Token::FUNC) {
            return callFunction(list, context);
        } else {
            std::cout << "Function call must start with valid function or operator\n";
            return item;
        }
    }
    
    if (item->type == Token::SYM) {
        // Unquoted symbol gets looked up
        return getVariable(item, context);
    }
    
    return item;
}

TokenPtr LispInterpreter::evaluate_list(TokenPtr list, ContextPtr context)
{
    TokenPtr ret;
    while (list) {
        ret = evaluate_item(list, context);
        list = list->next;
    }
    return ret;
}

TokenPtr LispInterpreter::evaluate_string(const std::string& s, ContextPtr context)
{
    TokenPtr p = parse_string(s);
    return evaluate_item(p, context);
}

static TokenPtr builtin_defun(TokenPtr list, ContextPtr context)
{
    std::cout << "defun: ";
    LispInterpreter::print_list(std::cout, list);
    std::cout << std::endl;
    
    if (list->type != Token::SYM) {
        printf("Function doesn't start with symbol name\n");
        return 0;
    }
    // XXX Check arg list
    TokenPtr t = std::make_shared<Token>();
    t->type = Token::FUNC;
    t->sym = list->sym;
    t->list = list->next;
    context->set(list->sym, t);
    return t;
}


static TokenPtr cat_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    return Token::make_string(interp, Token::string_val(a) + Token::string_val(b));
}

static TokenPtr add_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    a = Token::as_number(a);
    b = Token::as_number(b);
    if (a->type == Token::FLOAT || b->type == Token::FLOAT) {
        return Token::make_float(Token::float_val(a) + Token::float_val(b));
    } else {
        return Token::make_int(a->ival + b->ival);
    }
}

static TokenPtr mul_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    a = Token::as_number(a);
    b = Token::as_number(b);
    if (a->type == Token::FLOAT || b->type == Token::FLOAT) {
        return Token::make_float(Token::float_val(a) * Token::float_val(b));
    } else {
        return Token::make_int(a->ival * b->ival);
    }
}

static TokenPtr sub_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    a = Token::as_number(a);
    b = Token::as_number(b);
    if (a->type == Token::FLOAT || b->type == Token::FLOAT) {
        return Token::make_float(Token::float_val(a) - Token::float_val(b));
    } else {
        return Token::make_int(a->ival - b->ival);
    }
}

static TokenPtr div_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    a = Token::as_number(a);
    b = Token::as_number(b);
    if (a->type == Token::FLOAT || b->type == Token::FLOAT) {
        return Token::make_float(Token::float_val(a) / Token::float_val(b));
    } else {
        return Token::make_int(a->ival / b->ival);
    }
}

static TokenPtr mod_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    return Token::make_int(Token::int_val(a) % Token::int_val(b));
}

static TokenPtr eq_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    if (!a && !b) return Token::negone;
    if (!a) {
        if (b->type == Token::LIST) {
            return !b->list ? Token::negone : Token::zero;
        }
    }
    if (!b) {
        if (a->type == Token::LIST) {
            return !a->list ? Token::negone : Token::zero;
        }
    }
    
    if (a->type == Token::LIST && b->type == Token::LIST) {
        a = a->list;
        b = b->list;
        while (a && b) {
            TokenPtr c = eq_two(a, b, interp);
            if (!c->ival) return Token::zero;
        }
        if (a || b) return Token::zero;
        return Token::negone;
    }
    
    if (a->type == Token::SYM && b->type == Token::SYM) {
        return a->sym->index == b->sym->index ? Token::negone : Token::zero;
    }
    if (a->type == Token::SYM || b->type == Token::SYM) return Token::zero;
    
    if (a->type == Token::STR && b->type == Token::STR) {
        return a->sym->index == b->sym->index ? Token::negone : Token::zero;
    }
    if (a->type == Token::STR || b->type == Token::STR) {
        return Token::string_val(a) == Token::string_val(b) ? Token::negone : Token::zero;
    }
    
    if (a->type == Token::FLOAT || b->type == Token::FLOAT) {
        return Token::float_val(a) == Token::float_val(b) ? Token::negone : Token::zero;
    }

    if (a->type == Token::INT && b->type == Token::INT) {
        return a->ival == b->ival ? Token::negone : Token::zero;
    }
    
    return Token::zero;
}

static TokenPtr ne_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    TokenPtr c = eq_two(a, b, interp);
    return c->ival ? Token::zero : Token::negone;
}

static TokenPtr and_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    return Token::make_int(Token::int_val(a) & Token::int_val(b));
}

static TokenPtr or_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    return Token::make_int(Token::int_val(a) | Token::int_val(b));
}

static TokenPtr xor_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    return Token::make_int(Token::int_val(a) ^ Token::int_val(b));
}


typedef TokenPtr (*combine_f)(TokenPtr a, TokenPtr b, LispInterpreter *interp);


static TokenPtr oper_reduce_list(TokenPtr list, ContextPtr context, TokenPtr initial, combine_f comb)
{
    while (list) {
        TokenPtr item = context->interp->evaluate_item(list, context);
        initial = comb(initial, item, context->interp);
        list = list->next;
    }
    return initial;
}

static TokenPtr builtin_add(TokenPtr list, ContextPtr context)
{
    return oper_reduce_list(list, context, Token::zero, add_two);
}

static TokenPtr builtin_mul(TokenPtr list, ContextPtr context)
{
    return oper_reduce_list(list, context, Token::one, mul_two);
}

static TokenPtr builtin_cat(TokenPtr list, ContextPtr context)
{
    return oper_reduce_list(list, context, context->interp->empty_string, cat_two);
}

static TokenPtr builtin_and(TokenPtr list, ContextPtr context)
{
    return oper_reduce_list(list, context, Token::negone, and_two);
}

static TokenPtr builtin_or(TokenPtr list, ContextPtr context)
{
    return oper_reduce_list(list, context, Token::zero, or_two);
}

static TokenPtr builtin_xor(TokenPtr list, ContextPtr context)
{
    return oper_reduce_list(list, context, Token::zero, xor_two);
}

static TokenPtr oper_reduce_two(TokenPtr list, ContextPtr context, combine_f comb)
{
    TokenPtr a, b;
    
    if (list) {
        a = context->interp->evaluate_item(list, context);
        list = list->next;
    }
    if (list) {
        b = context->interp->evaluate_item(list, context);
    }
    
    return comb(a, b, context->interp);
}

static TokenPtr builtin_sub(TokenPtr list, ContextPtr context)
{
    return oper_reduce_two(list, context, sub_two);
}

static TokenPtr builtin_div(TokenPtr list, ContextPtr context)
{
    return oper_reduce_two(list, context, div_two);
}

static TokenPtr builtin_mod(TokenPtr list, ContextPtr context)
{
    return oper_reduce_two(list, context, mod_two);
}

static TokenPtr builtin_eq(TokenPtr list, ContextPtr context)
{
    return oper_reduce_two(list, context, eq_two);
}

static TokenPtr builtin_ne(TokenPtr list, ContextPtr context)
{
    return oper_reduce_two(list, context, ne_two);
}

static TokenPtr builtin_notzero(TokenPtr item, ContextPtr context)
{
    if (!item) return Token::zero;
    
    switch (item->type) {
    case Token::STR:
        {
            SymbolPtr s = item->sym;
            if (s->len == 0) return Token::zero;
            if (s->p[0] == '0') return Token::zero;
            return Token::negone;
        }
        
    case Token::INT:
        return item->ival ? Token::negone : Token::zero;
    
    case Token::FLOAT:
        return item->fval ? Token::negone : Token::zero;
        
    case Token::SYM:
        return Token::negone;
        
    case Token::LIST:
        return item->list ? Token::negone : Token::zero;
    }
    
    return Token::negone;
}

static TokenPtr builtin_not(TokenPtr item, ContextPtr context)
{
    TokenPtr c = builtin_notzero(item, context);
    return c->ival ? Token::zero : Token::negone;
}

static char precedence[][2] = {
    '+', 0,
    '-', 0,
    '*', 1,
    '/', 1,
};
static constexpr int num_precedence = sizeof(precedence) / sizeof(precedence[0]);

static int get_precedence(char op)
{
    for (int i=0; i<num_precedence; i++) {
        if (op == precedence[i][0]) return precedence[i][1];
    }
    return -1;
}

static TokenPtr builtin_infix(TokenPtr item, ContextPtr context)
{
    std::vector<TokenPtr> stack;
    TokenPtr l, c, n;
    int p1, p2;
    
    while (item) {
        switch (item->type) {
        case Token::INT:
        case Token::FLOAT:
        case Token::STR:
            if (!c) {
                c = Token::duplicate(item);
                l = c;
            } else {
                n = Token::duplicate(item);
                c->next = n;
                c = n;
            }
            break;
        case Token::SYM:
            p1 = get_precedence(*(item->sym->p));
            while (stack.size()) {
                TokenPtr o2 = stack.back();
                p2 = get_precedence(*(o2->sym->p));
                if (p2 < p1) break;
                stack.pop_back();
                if (!c) {
                    c = Token::duplicate(o2);
                    l = c;
                } else {
                    n = Token::duplicate(o2);
                    c->next = n;
                    c = n;
                }
            }
            stack.push_back(item);
            break;
        case Token::LIST:
            n = builtin_infix(item->list, context);
            if (!c) {
                c = n;
                l = c;
            } else {
                c->next = n;
                c = n;
            }            
            break;
        }
        
        item = item->next;
    }
    
    while (stack.size()) {
        TokenPtr o2 = stack.back();
        stack.pop_back();
        if (!c) {
            c = Token::duplicate(o2);
            l = c;
        } else {
            n = Token::duplicate(o2);
            c->next = n;
            c = n;
        }
    }
    
    return Token::as_list(l);
}

TokenPtr LispInterpreter::transform_infix(TokenPtr list)
{
    std::vector<TokenPtr> stack, queue;
    int p1, p2;
    
    if (list->type != Token::INFIX) return list;
    
    std::cout << "Infix list: ";
    print_list(std::cout, list);
    std::cout << std::endl;
    
    TokenPtr item = list->list;
    
    while (item) {
        switch (item->type) {
        case Token::INT:
        case Token::FLOAT:
        case Token::STR:
        case Token::LIST:
            queue.push_back(item);
            break;
        case Token::SYM:
            p1 = item->precedence;
            while (stack.size()) {
                TokenPtr o2 = stack.back();
                p2 = o2->precedence;
                if (p2 < p1) break;
                stack.pop_back();
                queue.push_back(o2);
            }
            stack.push_back(item);
            break;
        case Token::INFIX:
            queue.push_back(transform_infix(item->list));
            break;
        }
        
        item = item->next;
    }
    
    while (stack.size()) {
        TokenPtr o2 = stack.back();
        stack.pop_back();
        queue.push_back(o2);
    }
    
    for (int i=0; i+3<=queue.size();) {
        TokenPtr a = queue[i++];
        TokenPtr b = queue[i++];
        TokenPtr op = queue[i];
        op->next = a;
        a->next = b;
        b->next = 0;
        queue[i] = Token::as_list(op);
    }
    
    std::cout << "Transformed list: ";
    print_list(std::cout, queue.back());
    std::cout << std::endl;
    
    return queue.back();
}


int main() {
    // const char *floatStr = "3.14e2";
    // int length = strlen(floatStr);
    // float value = parse_float(floatStr, length);
    //
    // printf("Parsed value: %f\n", value);
    
    LispInterpreter li;
    li.addOperator("+", builtin_add, 0);
    li.addOperator("*", builtin_mul, 1);
    li.addOperator("-", builtin_sub, 0);
    li.addOperator("/", builtin_div, 1);
    li.addOperator("%", builtin_mod, 1);

    li.addOperator("&", builtin_and);
    li.addOperator("|", builtin_or);
    li.addOperator("^", builtin_xor);
    li.addOperator("and", builtin_and);
    li.addOperator("or", builtin_or);
    li.addOperator("xor", builtin_xor);
    li.addOperator("!!", builtin_notzero);
    li.addOperator("!", builtin_not);
    li.addOperator("not", builtin_not);
    
    li.addOperator("=", builtin_eq);
    li.addOperator("==", builtin_eq);
    li.addOperator("eq", builtin_eq);
    li.addOperator("<>", builtin_ne);
    li.addOperator("!=", builtin_ne);
    li.addOperator("ne", builtin_ne);
    
    li.addOperator("cat", builtin_cat);
    li.addOperator("defun", builtin_defun);
    
    li.addOperator("@", builtin_infix);
    
    for (std::string line; std::getline(std::cin, line);) {
        TokenPtr p = li.evaluate_string(line);
        li.print_list(std::cout, p);
        std::cout << std::endl;
    }
    
    // const char *sample = "('(x y) 4 '3.14 3.14e3 \"string\033xxx\") 123";
    //const char *sample = "(defun add4 (x) (+ x 4)) (add4 12)";
    //const char *sample = "(defun myadd (x y) (+ x y)) (myadd 13 8)";
    // const char *sample = "5";
    // TokenPtr p = li.evaluate_string(sample);
    // li.print_list(std::cout, p);
    // std::cout << std::endl;
    // // TokenPtr q = li.evaluate_list(p);
    // // li.print_list(std::cout, q);
    // std::cout << std::endl;
    return 0;
}
