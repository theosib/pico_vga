#include "lisp.hpp"

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

static TokenPtr pow_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    bool is_int = (a->type == Token::INT && b->type == Token::INT && b->ival >= 0);
    float af = Token::float_val(a);
    float bf = Token::float_val(b);
    float cf = pow(af, bf);
    // std::cout << "af=" << af << " bf=" << bf << " cf=" << cf << std::endl;
    if (is_int && isfinite(cf) && cf <= std::numeric_limits<int>::max() && cf >= std::numeric_limits<int>::min()) {
        return Token::make_int(cf);
    } else {
        return Token::make_float(cf);
    }
}

static TokenPtr lt_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    if (!a && !b) return Token::zero;
    if (!a) {
        if (b->type == Token::LIST || b->type == Token::INFIX) {
            return !b->list ? Token::negone : Token::zero;
        } else {
            return Token::negone;
        }
    }
    if (!b) {
        if (a->type == Token::LIST || a->type == Token::INFIX) {
            return !a->list ? Token::zero : Token::negone;
        } else {
            return Token::zero;
        }
    }
    
    if ((a->type == Token::LIST || a->type == Token::INFIX) && (b->type == Token::LIST || b->type == Token::INFIX)) {
        a = a->list;
        b = b->list;
        // Make sure at least one is lessthan
        while (a && b) {
            TokenPtr c = lt_two(a, b, interp);
            if (!c->ival) return Token::zero;
        }
        if (a || b) return Token::zero;
        return Token::negone;
    }
    
    if (a->type == Token::SYM && b->type == Token::SYM) {
        return a->sym->index == b->sym->index ? Token::zero : Token::negone;
    }
    if (a->type == Token::SYM || b->type == Token::SYM) return Token::zero;
    
    if (a->type == Token::STR && b->type == Token::STR) {
        if (a->sym->index == b->sym->index) return Token::zero;
    }
    if (a->type == Token::STR || b->type == Token::STR) {
        int c = strcmp(Token::string_val(a).c_str(), Token::string_val(b).c_str());
        return (c < 0) ? Token::negone : Token::zero;
    }
    
    if (a->type == Token::FLOAT || b->type == Token::FLOAT) {
        return Token::float_val(a) < Token::float_val(b) ? Token::negone : Token::zero;
    }

    if (a->type == Token::INT && b->type == Token::INT) {
        return a->ival < b->ival ? Token::negone : Token::zero;
    }
    
    return Token::zero;
}

static TokenPtr gt_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    return lt_two(b, a, interp);
}

static TokenPtr le_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    return lt_two(b, a, interp)->ival ? Token::zero : Token::negone;
}

static TokenPtr ge_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    return lt_two(a, b, interp)->ival ? Token::zero : Token::negone;
}

static TokenPtr eq_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    if (!a && !b) return Token::negone;
    if (!a) {
        if (b->type == Token::LIST || b->type == Token::INFIX) {
            return !b->list ? Token::negone : Token::zero;
        } else {
            return Token::zero;
        }
    }
    if (!b) {
        if (a->type == Token::LIST || a->type == Token::INFIX) {
            return !a->list ? Token::negone : Token::zero;
        } else {
            return Token::zero;
        }
    }
    
    if ((a->type == Token::LIST || a->type == Token::INFIX) && (b->type == Token::LIST || b->type == Token::INFIX)) {
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

static TokenPtr builtin_pow(TokenPtr list, ContextPtr context)
{
    return oper_reduce_two(list, context, pow_two);
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

static TokenPtr builtin_lt(TokenPtr list, ContextPtr context)
{
    return oper_reduce_two(list, context, lt_two);
}

static TokenPtr builtin_gt(TokenPtr list, ContextPtr context)
{
    return oper_reduce_two(list, context, gt_two);
}

static TokenPtr builtin_le(TokenPtr list, ContextPtr context)
{
    return oper_reduce_two(list, context, le_two);
}

static TokenPtr builtin_ge(TokenPtr list, ContextPtr context)
{
    return oper_reduce_two(list, context, ge_two);
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


void LispInterpreter::loadOperators()
{
    addOperator("+", builtin_add, 4);
    addOperator("-", builtin_sub, 4);
    addOperator("*", builtin_mul, 3);
    addOperator("/", builtin_div, 3);
    addOperator("%", builtin_mod, 3);
    addOperator("**", builtin_pow, 1, Token::RASSOC);

    addOperator("&", builtin_and, 8);
    addOperator("&&", builtin_and, 8);
    addOperator("|", builtin_or, 9);
    addOperator("||", builtin_or, 9);
    addOperator("^", builtin_xor, 10);
    addOperator("and", builtin_and, 8);
    addOperator("or", builtin_or, 8);
    addOperator("xor", builtin_xor, 10);
    addOperator("!!", builtin_notzero, 2, Token::UNARY);
    addOperator("!", builtin_not, 2, Token::UNARY);
    addOperator("not", builtin_not, 2, Token::UNARY);
    
    addOperator("=", builtin_eq, 7);
    addOperator("==", builtin_eq, 7);
    addOperator("eq", builtin_eq, 7);
    addOperator("<>", builtin_ne, 7);
    addOperator("!=", builtin_ne, 7);
    addOperator("ne", builtin_ne, 7);

    addOperator("<", builtin_lt, 6);
    addOperator(">", builtin_gt, 6);
    addOperator("<=", builtin_le, 6);
    addOperator(">=", builtin_ge, 6);
    
    addOperator("cat", builtin_cat);
    addOperator("defun", builtin_defun);
}
