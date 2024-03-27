#ifndef INCLUDED_LISP_HPP
#define INCLUDED_LISP_HPP

#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <vector>
#include <unordered_map>
#include <string_view>
#include <iostream>
#include <memory>

struct Symbol;
typedef std::shared_ptr<Symbol> SymbolPtr;
typedef std::weak_ptr<Symbol> SymbolWeakPtr;

struct Symbol : public std::enable_shared_from_this<Symbol> {
    char *p = 0;
    int len = 0;
    int index = 0;
    Symbol() {}
    Symbol(char *p, int len, int ix) {
        this->p = p;
        this->len = len;
        this->index = ix;
    }
    ~Symbol() {
        delete [] p;
    }
    
    SymbolPtr real;
    SymbolPtr next;
    
    std::string_view as_stringview() const {
        return real ? real->as_stringview() : std::string_view(p, len);
    }

    std::ostream& print_item(std::ostream& os) const {
        os << as_stringview();
        return os;
    }
    
    SymbolPtr wrap() {
        SymbolPtr s = std::make_shared<Symbol>();
        s->real = shared_from_this();
        s->index = index;
        return s;
    }
};

inline std::ostream& operator<<(std::ostream& os, const Symbol& s) {
    //os << ':' << std::string_view(s.p, s.len) << '[' << s.index << ']';
    s.print_item(os);
    const SymbolPtr rest = s.next;
    if (rest) {
        os << ':' << *rest;
    }
    return os;
}


inline std::ostream& operator<<(std::ostream& os, const SymbolPtr& s) {
    if (!s) {
        os << "[noname]";
    } else {
        os << *s;
    }
    return os;
}

struct Interns {
    std::vector<SymbolWeakPtr> syms;    
    SymbolPtr find(const char *p, int len);
    SymbolPtr find(const std::string& s) {
        return find(s.data(), s.length());
    }
    SymbolPtr find(const std::string_view& s) {
        return find(s.data(), s.length());
    }
};

class LispInterpreter;
struct Token;
typedef std::shared_ptr<Token> TokenPtr;
struct Context;
typedef std::shared_ptr<Context> ContextPtr;
typedef TokenPtr (*built_in_f)(TokenPtr, ContextPtr);
struct Parsing;

struct Token {
    enum {
        LASSOC,
        RASSOC,
        UNARY
    };
    
    enum {
        NONE,
        LIST,
        INT,
        STR,
        FLOAT,
        CHAR,
        OPER,
        SYM,
        ARR,
        FUNC,
        INFIX,
        BOOL,
        CLASS,
        OBJECT,
        EXCEPTION
    };
    
    uint8_t type;
    uint8_t quote;
    uint8_t precedence;
    uint8_t order;
    TokenPtr list; // also array
    SymbolPtr sym;  // symbols and strings
    ContextPtr context; // Classes, objects, lambdas, exceptions
    union {
        int32_t ival; 
        float fval;
        built_in_f oper;
    };
    
    TokenPtr next;
    
    Token() {
        next = 0;
        type = 0;
        quote = 0;
        ival = 0;
        precedence = 0;
        order = 0;
    }
    
    ~Token() {}
    
    static TokenPtr as_list(TokenPtr head) {
        TokenPtr l = std::make_shared<Token>();
        l->type = LIST;
        l->list = head;
        return l;
    }
    
    static TokenPtr duplicate(TokenPtr item) {
        if (!item) return 0;
        TokenPtr l = std::make_shared<Token>();
        l->type = item->type;
        l->quote = item->quote;
        l->precedence = item->precedence;
        l->order = item->order;
        l->list = item->list;
        l->sym = item->sym;
        l->oper = item->oper; // ival, etc.
        return l;
    }
    
    static TokenPtr zero, one, negone, bool_true, bool_false;
    
    static TokenPtr make_int(int val) {
        TokenPtr l = std::make_shared<Token>();
        l->type = INT;
        l->ival = val;
        return l;
    }

    static TokenPtr make_float(float val) {
        TokenPtr l = std::make_shared<Token>();
        l->type = FLOAT;
        l->fval = val;
        return l;
    }
    
    static TokenPtr make_bool(int val) {
        TokenPtr l = std::make_shared<Token>();
        l->type = BOOL;
        l->ival = !!val;
        return l;
    }
    
    static TokenPtr make_string(LispInterpreter *interp, const char *p, int len);
    static TokenPtr make_string(LispInterpreter *interp, const std::string& s) {
        return make_string(interp, s.data(), s.length());
    }
    static TokenPtr make_string(LispInterpreter *interp, const std::string_view& s) {
        return make_string(interp, s.data(), s.length());
    }
        
    static TokenPtr as_number(TokenPtr item);    
    static TokenPtr as_int(TokenPtr item);
    static TokenPtr as_float(TokenPtr item);    
    static TokenPtr as_string(LispInterpreter *interp, TokenPtr item);
    static TokenPtr as_bool(TokenPtr item);
    
    static float float_val(TokenPtr item);
    static int int_val(TokenPtr item);
    static std::string inspect(TokenPtr item);
    static std::string string_val(TokenPtr item);
    static bool bool_val(TokenPtr item);
    
    static std::ostream& print_octal(std::ostream& os, int val, int len);
    static std::ostream& print_string(std::ostream& os, const char *s, int len);
    static int parse_octal(const char *src, int len);
    static int parse_decimal(const char *src, int len);
    static int parse_hex(const char *src, int len);
    static int parse_string(const char *src, int len_in, char *dst);
    static float parse_float(const char *str, int length);
    static TokenPtr parse_number(Parsing& p);
    
};

std::ostream& operator<<(std::ostream& os, const TokenPtr& s);

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
    void rewind() {
        len += p - q;
        p = q;
    }
    
    Parsing() {}
    Parsing(const char *ptr, int l) : p(ptr), len(l) {}
};

struct Dictionary {
    std::unordered_map<int, std::pair<SymbolPtr, TokenPtr>> entries;
    void set(SymbolPtr s, TokenPtr t) {
        std::cout << "Setting " << s << " to " << t << std::endl;
        entries[s->index] = {s, t};
    }
    void unset(SymbolPtr s) {
        entries.erase(s->index);
    }
    TokenPtr get(SymbolPtr s) {
        auto i = entries.find(s->index);
        if (i == entries.end()) {
            std::cout << "Variable " << s << " not found\n";
            return 0;
        }
        TokenPtr r = i->second.second;
        std::cout << "Getting " << s << " as " << r << std::endl;
        return r;
    }
};

typedef std::shared_ptr<Dictionary> DictionaryPtr;


struct Context : public std::enable_shared_from_this<Context> {
    LispInterpreter *interp;
    ContextPtr parent;
    Dictionary vars;
    
    SymbolPtr name;
    uint8_t type = 0;
    
    void set(SymbolPtr s, TokenPtr t) {
        vars.set(s, t);
    }
    void set_global(SymbolPtr s, TokenPtr t);
    ContextPtr get_owner(SymbolPtr& s);
    TokenPtr get(SymbolPtr& s, ContextPtr& owner);
    TokenPtr get(SymbolPtr s) {
        ContextPtr owner;
        return get(s, owner);
    }
    TokenPtr get_local(SymbolPtr s);
    
    ContextPtr make_child() {
        ContextPtr p = std::make_shared<Context>(interp);
        p->parent = shared_from_this();
        return p;
    }
    
    ContextPtr make_child_class(SymbolPtr name) {
        ContextPtr p = std::make_shared<Context>(interp);
        p->parent = shared_from_this();
        p->name = name;
        p->type = Token::CLASS;
        return p;
    }
    
    ContextPtr make_child_function(SymbolPtr name) {
        ContextPtr p = std::make_shared<Context>(interp);
        p->parent = shared_from_this();
        p->name = name;
        p->type = Token::FUNC;
        return p;
    }
    
    ContextPtr make_child_object() {
        ContextPtr p = std::make_shared<Context>(interp);
        p->parent = shared_from_this();
        p->type = Token::OBJECT;
        return p;
    }
    
    TokenPtr make_exception(const std::string err);
    
    TokenPtr make_exception(TokenPtr child) {
        TokenPtr t = std::make_shared<Token>();
        t->type = Token::EXCEPTION;
        t->context = shared_from_this();
        t->next = child;        
        return t;
    }
    
    TokenPtr check_exception(TokenPtr child) {
        if (child->type == Token::EXCEPTION) {
            return make_exception(child);
        } else {
            return child;
        }
    }
    
    std::string get_full_path(SymbolPtr name);
    
    Context(LispInterpreter *i) : interp(i) {}
};


struct LispInterpreter {
    Interns interns;
    // XXX Represent the global context as an object so that it has a name
    ContextPtr globals = std::make_shared<Context>(this);    
    TokenPtr empty_string = Token::make_string(this, "", 0);
    
    // std::string get_full_path(SymbolPtr name, ContextPtr context = 0) {
    //     if (!context) context = globals;
    //     return context->get_full_path(name);
    // }
    
    SymbolPtr find_symbol(const std::string& s) {
        return interns.find(s.data(), s.length());
    }
    
    SymbolPtr parse_symbol(const char *str, int len);
    
    // XXX rename "string" to "code" or something
    TokenPtr parse_string(Parsing& p);
    TokenPtr parse_string(const std::string& s) {
        Parsing pa(s.data(), s.length());
        return Token::as_list(parse_string(pa));
    }
    TokenPtr parse_token(Parsing& p);
    TokenPtr transform_infix(TokenPtr list);
    
    static std::ostream& print_list(std::ostream& os, TokenPtr head, ContextPtr context = 0);
    static std::ostream& print_item(std::ostream& os, TokenPtr item, ContextPtr context = 0);
    
    TokenPtr evaluate_item(TokenPtr item, ContextPtr context = 0);
    TokenPtr evaluate_list(TokenPtr item, ContextPtr context = 0);
    void addFunction(TokenPtr list, ContextPtr context);
    // void setVariable(TokenPtr list, ContextPtr context);
    TokenPtr getVariable(TokenPtr name, ContextPtr context);    
    TokenPtr callFunction(SymbolPtr name, TokenPtr args, TokenPtr params, ContextPtr owner, ContextPtr caller);
    
    TokenPtr evaluate_string(const std::string& name, ContextPtr context = 0);

    void addOperator(const std::string& name, built_in_f f, int precedence = 0, int order = 0);
    void loadOperators();
    
    LispInterpreter() {
        loadOperators();
    }
};


// inline std::ostream& operator<<(std::ostream& os, const TokenPtr& s) {
//     os << ':' << std::string_view(s.p, s.len) << '[' << s.index << ']';
//     return os;
// }
//
// inline std::ostream& operator<<(ostream& os, const SymbolPtr& s) {
//     os << *s;
//     return os;
// }


#endif
