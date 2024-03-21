
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
        case Token::INT:
        case Token::FLOAT:
        case Token::STR:
        case Token::LIST:
            queue.push_back(item);
            break;
        case Token::SYM:
            oper = globals->get(item->sym);
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
    
    std::cout << "Queue: ";
    for (int i=0; i<queue.size(); i++) {
        print_item(std::cout, queue[i]);
        std::cout << " ";
    }
    std::cout << std::endl;
    
    stack.clear();
    
    for (int i=0; i<queue.size(); i++) {
        TokenPtr c = queue[i];
        if (c->type == Token::SYM) {
            if (c->order == Token::UNARY) {
                TokenPtr a = stack.back(); stack.pop_back();
                c->next = a;
                a->next = 0;
            } else {
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
