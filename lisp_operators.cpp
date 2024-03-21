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

static TokenPtr eq_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    if (!a && !b) return Token::bool_true;
    if (!a) {
        if (b->type == Token::LIST || b->type == Token::INFIX) {
            return !b->list ? Token::bool_true : Token::bool_false;
        } else {
            return Token::bool_false;
        }
    }
    if (!b) {
        if (a->type == Token::LIST || a->type == Token::INFIX) {
            return !a->list ? Token::bool_true : Token::bool_false;
        } else {
            return Token::bool_false;
        }
    }
    
    if ((a->type == Token::LIST || a->type == Token::INFIX) && (b->type == Token::LIST || b->type == Token::INFIX)) {
        a = a->list;
        b = b->list;
        while (a && b) {
            TokenPtr c = eq_two(a, b, interp);
            if (!c->ival) return Token::bool_false;
        }
        if (a || b) return Token::bool_false;
        return Token::bool_true;
    }
    
    if (a->type == Token::SYM && b->type == Token::SYM) {
        return a->sym->index == b->sym->index ? Token::bool_true : Token::bool_false;
    }
    if (a->type == Token::SYM || b->type == Token::SYM) return Token::bool_false;
    
    if (a->type == Token::STR && b->type == Token::STR) {
        return a->sym->index == b->sym->index ? Token::bool_true : Token::bool_false;
    }
    if (a->type == Token::STR || b->type == Token::STR) {
        return Token::string_val(a) == Token::string_val(b) ? Token::bool_true : Token::bool_false;
    }
    
    if (a->type == Token::FLOAT || b->type == Token::FLOAT) {
        return Token::float_val(a) == Token::float_val(b) ? Token::bool_true : Token::bool_false;
    }

    if ((a->type == Token::INT || a->type == Token::BOOL) && (b->type == Token::INT || b->type == Token::BOOL)) {
        return a->ival == b->ival ? Token::bool_true : Token::bool_false;
    }
    
    return Token::bool_false;
}

static TokenPtr lt_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    if (!a && !b) return Token::bool_false;
    if (!a) {
        if (b->type == Token::LIST || b->type == Token::INFIX) {
            return !b->list ? Token::bool_true : Token::bool_false;
        } else {
            return Token::bool_true;
        }
    }
    if (!b) {
        if (a->type == Token::LIST || a->type == Token::INFIX) {
            return !a->list ? Token::bool_false : Token::bool_true;
        } else {
            return Token::bool_false;
        }
    }
    
    if ((a->type == Token::LIST || a->type == Token::INFIX) && (b->type == Token::LIST || b->type == Token::INFIX)) {
        a = a->list;
        b = b->list;
        bool got_lt = false;
        while (a && b) {
            TokenPtr c = lt_two(a, b, interp);
            if (c->ival) {
                got_lt = true;
            } else {
                c = eq_two(a, b, interp);
                if (!c->ival) return Token::bool_false;
            }
        }
        if (a) return Token::bool_false;
        if (b) return Token::bool_true;
        return got_lt ? Token::bool_true : Token::bool_false;
    }
    
    if (a->type == Token::SYM && b->type == Token::SYM) {
        return a->sym->index == b->sym->index ? Token::bool_false : Token::bool_true;
    }
    if (a->type == Token::SYM || b->type == Token::SYM) return Token::bool_false;
    
    if (a->type == Token::STR && b->type == Token::STR) {
        if (a->sym->index == b->sym->index) return Token::bool_false;
    }
    if (a->type == Token::STR || b->type == Token::STR) {
        int c = strcmp(Token::string_val(a).c_str(), Token::string_val(b).c_str());
        return (c < 0) ? Token::bool_true : Token::bool_false;
    }
    
    if (a->type == Token::FLOAT || b->type == Token::FLOAT) {
        return Token::float_val(a) < Token::float_val(b) ? Token::bool_true : Token::bool_false;
    }

    if ((a->type == Token::INT || a->type == Token::BOOL) && (b->type == Token::INT || b->type == Token::BOOL)) {
        return a->ival < b->ival ? Token::bool_true : Token::bool_false;
    }
    
    return Token::bool_false;
}

static TokenPtr gt_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    return lt_two(b, a, interp);
}

static TokenPtr le_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    return lt_two(b, a, interp)->ival ? Token::bool_false : Token::bool_true;
}

static TokenPtr ge_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    return lt_two(a, b, interp)->ival ? Token::bool_false : Token::bool_true;
}

static TokenPtr ne_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    TokenPtr c = eq_two(a, b, interp);
    return c->ival ? Token::bool_false : Token::bool_true;
}

static TokenPtr and_two_bitwise(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    return Token::make_int(Token::int_val(a) & Token::int_val(b));
}

static TokenPtr and_two_bool(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    return (Token::bool_val(a) && Token::bool_val(b)) ? Token::bool_true : Token::bool_false;
}

static TokenPtr or_two(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    return Token::make_int(Token::int_val(a) | Token::int_val(b));
}

static TokenPtr or_two_bitwise(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    return Token::make_int(Token::int_val(a) | Token::int_val(b));
}

static TokenPtr or_two_bool(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    return (Token::bool_val(a) || Token::bool_val(b)) ? Token::bool_true : Token::bool_false;
}

static TokenPtr xor_two_bitwise(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    return Token::make_int(Token::int_val(a) ^ Token::int_val(b));
}

static TokenPtr xor_two_bool(TokenPtr a, TokenPtr b, LispInterpreter *interp)
{
    return (Token::bool_val(a) != Token::bool_val(b)) ? Token::bool_true : Token::bool_false;
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

static TokenPtr builtin_and_bitwise(TokenPtr list, ContextPtr context)
{
    return oper_reduce_list(list, context, Token::negone, and_two_bitwise);
}

static TokenPtr builtin_and_bool(TokenPtr list, ContextPtr context)
{
    return oper_reduce_list(list, context, Token::bool_true, and_two_bool);
}

static TokenPtr builtin_or_bitwise(TokenPtr list, ContextPtr context)
{
    return oper_reduce_list(list, context, Token::zero, or_two_bitwise);
}

static TokenPtr builtin_or_bool(TokenPtr list, ContextPtr context)
{
    return oper_reduce_list(list, context, Token::bool_false, or_two_bool);
}

static TokenPtr builtin_xor_bitwise(TokenPtr list, ContextPtr context)
{
    return oper_reduce_list(list, context, Token::zero, xor_two_bitwise);
}

static TokenPtr builtin_xor_bool(TokenPtr list, ContextPtr context)
{
    return oper_reduce_list(list, context, Token::bool_false, xor_two_bool);
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
    return Token::as_bool(item);
}

static TokenPtr builtin_not_bool(TokenPtr item, ContextPtr context)
{
    return Token::bool_val(item) ? Token::bool_false : Token::bool_true;
}

static TokenPtr builtin_not_bitwise(TokenPtr item, ContextPtr context)
{
    return Token::make_int(~Token::int_val(item));
}

static TokenPtr builtin_neg(TokenPtr item, ContextPtr context)
{
    TokenPtr a = Token::as_number(item);
    if (a->type == Token::FLOAT) {
        return Token::make_float(-a->fval);
    } else {
        return Token::make_int(-a->ival);
    }
}

static TokenPtr builtin_identity(TokenPtr item, ContextPtr context)
{
    return item;
}

void LispInterpreter::loadOperators()
{
    globals->set(find_symbol("true"), Token::bool_true);
    globals->set(find_symbol("false"), Token::bool_false);
    
    addOperator("+", builtin_add, 4);
    addOperator("-", builtin_sub, 4);
    addOperator("*", builtin_mul, 3);
    addOperator("/", builtin_div, 3);
    addOperator("%", builtin_mod, 3);
    addOperator("**", builtin_pow, 1, Token::RASSOC);

    addOperator("&", builtin_and_bitwise, 8);
    addOperator("^", builtin_xor_bitwise, 9);
    addOperator("|", builtin_or_bitwise, 10);
    
    addOperator("&&", builtin_and_bool, 11);
    addOperator("and", builtin_and_bool, 11);
    addOperator("||", builtin_or_bool, 12);
    addOperator("or", builtin_or_bool, 12);
    addOperator("xor", builtin_or_bool, 13);
    
    addOperator("!!", builtin_notzero, 2, Token::UNARY);
    addOperator("!", builtin_not_bool, 2, Token::UNARY);
    addOperator("not", builtin_not_bool, 2, Token::UNARY);
    addOperator("~", builtin_not_bitwise, 2, Token::UNARY);
    addOperator("neg", builtin_neg, 2, Token::UNARY);
    
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
    
    addOperator("cat", builtin_cat, 0);
    addOperator("identity", builtin_identity, 0, Token::UNARY);
    addOperator("defun", builtin_defun);
    
    // int, floor, ceil, round, float, bool, bitwise operators, bitwise not
    // Adding to item list requires duplication if next is not null
    
}
