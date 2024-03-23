
#include "lisp.hpp"
#include <sstream>



std::ostream& operator<<(std::ostream& os, const TokenPtr& s)
{
    LispInterpreter::print_item(os, s);
    return os;
}


std::string Context::get_full_path(SymbolPtr name)
{
    std::string s;
    if (name) s = name->as_stringview();

    if (this->name) {
        s = std::string(this->name->as_stringview()) + std::string(":") + s;
    } else {
        if (parent) {
            s = "local:" + s;
        } else {
            return "global:" + s;
        }
    }
    
    return parent->get_full_path(0) + ':' + s;
}

void Context::set_global(SymbolPtr s, TokenPtr t)
{
    if (!parent) {
        set(s, t);
    } else {
        parent->set_global(s, t);
    }
}

ContextPtr Context::get_owner(SymbolPtr& name)
{
    ContextPtr owner = shared_from_this();
    while (name->next) {
        TokenPtr t = owner->get_local(name);
        if (!t) {
            std::cout << "No such variable or context ";
            name->print_item(std::cout);
            std::cout << std::endl;
            return 0;
        }
        if (t->type != Token::OBJECT && t->type != Token::CLASS) {
            std::cout << "Variable ";
            name->print_item(std::cout);
            std::cout << " is not of type object or class\n";
            return 0;
        }
        
        std::cout << "Owner of " << name << " is " << t->sym << std::endl;
        
        owner = t->context;
        name = name->next;
    }
    return owner;
}

TokenPtr Context::get_local(SymbolPtr name)
{
    return vars.get(name);
}

TokenPtr Context::get(SymbolPtr& name, ContextPtr& owner)
{
    owner = get_owner(name);    
    TokenPtr t = owner->vars.get(name);
    if (t) return t;
    if (!owner->parent) return 0;
    if (owner->type == Token::OBJECT) {
        ContextPtr dummy;
        return owner->parent->get(name, dummy);
    } else {
        return owner->parent->get(name, owner);
    }
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

TokenPtr Token::zero = Token::make_int(0);
TokenPtr Token::one = Token::make_int(1);
TokenPtr Token::negone = Token::make_int(-1);
TokenPtr Token::bool_true = Token::make_bool(true);
TokenPtr Token::bool_false = Token::make_bool(false);

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
    if (item->type == BOOL) return make_bool(item->ival);
    if (item->type == INT || item->type == FLOAT) return item;
    if (item->type == STR) {
        Parsing p(item->sym->p, item->sym->len);
        TokenPtr t = parse_number(p);
        if (t->type == INT || t->type == FLOAT) return t;
    }
    return zero;
}

TokenPtr Token::as_bool(TokenPtr item)
{
    if (!item) return bool_false;
    if (item->type == BOOL) return item;
    if (item->type == INT) return (item->ival ? bool_true : bool_false);
    if (item->type == FLOAT) return (item->fval ? bool_true : bool_false);
    if (item->type == STR) return (item->sym->len && item->sym->p[0]!='0') ? bool_true : bool_false;
    if (item->type == SYM) return bool_true;
    if (item->type == LIST || item->type == INFIX) return (item->list ? bool_true : bool_false);
    return bool_false;
}

bool Token::bool_val(TokenPtr item)
{
    return as_bool(item)->ival;
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
    if (item->type == INT || item->type == BOOL) return item->ival;
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
    if (item->type == INT || item->type == BOOL) return item->ival;
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

void LispInterpreter::addOperator(const std::string& name, built_in_f f, int precedence, int order)
{
    SymbolPtr s = interns.find(name.data(), name.length());
    TokenPtr t = std::make_shared<Token>();
    t->type = Token::OPER;
    t->oper = f;
    t->sym = s;
    t->precedence = precedence;
    t->order = order;
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


TokenPtr LispInterpreter::callFunction(SymbolPtr name, TokenPtr args, TokenPtr params, ContextPtr owner, ContextPtr caller)
{
    std::cout << "Calling func " << name << ": ";
    print_list(std::cout, params);
    std::cout << std::endl;
    
    TokenPtr body = params->next;
    if (!body) return 0; // Empty body is okay, but we need to do no more work
            
    if (params->type != Token::LIST) {
        std::cout << "Function doesn't start with list of args\n";
        return 0;
    }
    
    // Make a local variable context
    owner = owner->make_child();
    
    params = params->list;
    while (args && params) {
        // XXX handle wrong number of arguments
        SymbolPtr param_name = params->sym;
        if (params->quote) {
            // Final element, gets rest of args as list
            // This needs to make a list out of evaluated items
            // XXX Evaluate each from the calling context
            owner->set(param_name, Token::as_list(args));
            break;
        } else {
            // Evaluate argument in the calling context and put the result into the callee context
            owner->set(param_name, evaluate_item(args, caller));
        }
        params = params->next;
        args = args->next;
    }
    
    return evaluate_list(body, owner);
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
TokenPtr LispInterpreter::evaluate_item(TokenPtr item, ContextPtr caller)
{
    if (!item) return 0;
    
    std::cout << "Evaluating: ";
    print_item(std::cout, item, caller);
    std::cout << std::endl;
    
    if (item->quote) return item;    
    if (!caller) caller = globals;
    
    if (item->type == Token::LIST) {
        // Unquoted list is function or operator call or list of calls
        TokenPtr list = item->list;
        
        TokenPtr ret;
        while (list && list->type == Token::LIST) {
            ret = evaluate_item(list, caller);
            list = list->next;
        }
        
        if (!list) return ret;
        
        if (list->type != Token::SYM) {
            std::cout << "Function call must start with symbol\n";
            return item;
        }
        
        SymbolPtr name = list->sym;
        TokenPtr args = list->next;
        ContextPtr owner;
        TokenPtr func = caller->get(name, owner);
        if (!func) {
            std::cout << "Function call failed\n";
            // Pass on the exception
            return 0;
        }
        
        if (func->type == Token::CLASS) {
            TokenPtr obj = std::make_shared<Token>();
            obj->type = Token::OBJECT;
            obj->sym = name;
            obj->context = func->context->make_child_object(name); // Parent of object is class
            // See if class has "_init" method
            SymbolPtr _init_sym = find_symbol("_init");
            TokenPtr _init_func = func->context->get_local(_init_sym);
            if (_init_func) {
                if (_init_func->type != Token::FUNC) {
                    std::cout << "Error: _init must be function\n";
                } else {
                    TokenPtr params_body = _init_func->list;
                    callFunction(_init_sym, args, params_body, obj->context, caller);
                }
            }
            return obj;
        } else if (func->type == Token::OPER) {
            return (*func->oper)(list->next, caller);
        } else if (func->type == Token::FUNC) {
            TokenPtr params_body = func->list;
            return callFunction(name, args, params_body, owner, caller);
        } else {
            std::cout << "Function call must start with valid function or operator\n";
            return item;
        }
    }
    
    if (item->type == Token::SYM) {
        // Unquoted symbol gets looked up
        return getVariable(item, caller);
    }
    
    return item;
}

// Evaluates a function body -- rename to evaluate_rest
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

TokenPtr LispInterpreter::transform_infix(TokenPtr list)
{
    std::vector<TokenPtr> stack, queue;
    int p1, p2;
    TokenPtr oper;
    
    if (list->type != Token::INFIX) return list;
    
    std::cout << "Infix list: ";
    print_list(std::cout, list);
    std::cout << std::endl;
    
    TokenPtr item = list->list;
    
    while (item) {
        switch (item->type) {
        case Token::SYM:
            oper = globals->get(item->sym);
            if (oper && oper->type == Token::OPER) {
                item->precedence = oper->precedence;
                item->order = oper->order;
                p1 = item->precedence;
                std::cout << "p1=" << p1 << std::endl;
                while (stack.size()) {
                    TokenPtr o2 = stack.back();
                    p2 = o2->precedence;
                    std::cout << "p2=" << p2 << std::endl;
                    if ((o2->order == Token::LASSOC && p2 > p1) || (o2->order != Token::LASSOC && p2 >= p1)) break;
                    stack.pop_back();
                    queue.push_back(o2);
                }
                // Make sure this item is marked as an operator
                if (!item->precedence) item->precedence = 1;
            }
            stack.push_back(item);
            break;
        case Token::INFIX:
            queue.push_back(transform_infix(item->list));
            break;
        default:
            queue.push_back(item);
            break;
        }
        
        item = item->next;
    }
    
    while (stack.size()) {
        TokenPtr o2 = stack.back();
        stack.pop_back();
        queue.push_back(o2);
    }
    
    std::cout << "Queue: ";
    for (int i=0; i<queue.size(); i++) {
        print_item(std::cout, queue[i]);
        std::cout << " ";
    }
    std::cout << std::endl;
    
    stack.clear();
    
    if (queue.size() < 2) {
        TokenPtr item = Token::make_string(this, std::string_view("identity"));
        item->type = Token::SYM;
        item->precedence = 1;
        item->order = Token::UNARY;
        queue.push_back(item);
    }
    
    for (int i=0; i<queue.size(); i++) {
        TokenPtr c = queue[i];
        if (c->type == Token::SYM && c->precedence) {
            if (c->order == Token::UNARY) {
                TokenPtr a = stack.back(); stack.pop_back();
                c->next = a;
                a->next = 0;
            } else {
                if (stack.size() < 2) return 0;
                TokenPtr b = stack.back(); stack.pop_back();
                TokenPtr a = stack.back(); stack.pop_back();
                c->next = a;
                a->next = b;
                b->next = 0;
            }
            stack.push_back(Token::as_list(c));
        } else {
            stack.push_back(c);
        }
    }
    
    std::cout << "Transformed list: ";
    print_list(std::cout, stack.back());
    std::cout << std::endl;
    
    return stack.back();
}

/*
    evaluate_each -- produce list of evaluated entries of a list
*/

int main() {
    // const char *floatStr = "3.14e2";
    // int length = strlen(floatStr);
    // float value = parse_float(floatStr, length);
    //
    // printf("Parsed value: %f\n", value);
    
    LispInterpreter li;
    
    
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
