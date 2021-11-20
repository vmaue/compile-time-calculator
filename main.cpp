#include <variant>
#include <vector>

namespace {
    //https://stackoverflow.com/questions/2033110/passing-a-string-literal-as-a-type-argument-to-a-class-template
    template<std::size_t N>
    struct FixedString {
        char buf[N + 1]{};
        constexpr FixedString(char const* s) {
            for (std::size_t i = 0; i < N; ++i) buf[i] = s[i];
        }
        constexpr operator char const*() const { return buf; }
        constexpr std::size_t size() const {return N;}
    };
    template<std::size_t N> FixedString(char const (&)[N]) -> FixedString<N - 1>;

    //https://www.youtube.com/watch?v=EsUmnLgz8QY
    template<typename ...Base>
    struct Visitor : Base ... {
        using Base::operator()...;
    };

    template<typename ...T>
    Visitor(T...) -> Visitor<T...>;

    struct Number {int value;
        int operator()([[maybe_unused]] int a, [[maybe_unused]] int b) const { throw; }};
    struct Plus{constexpr int operator()(int a, int b) const { return a + b; }};
    struct Minus{constexpr int operator()(int a, int b) const { return a - b; }};
    struct Mult{constexpr int operator()(int a, int b) const { return a * b; }};
    struct Div{constexpr int operator()(int a, int b) const { return a / b; }};
    struct LeftPar{int operator()([[maybe_unused]] int a, [[maybe_unused]] int b) const { throw; }};

    enum type {
        NUMBER,
        PLUS,
        MINUS,
        MULT,
        DIV,
        LEFTPAR
    };

    using rpn_variant = std::variant<Number, Plus, Minus, Mult, Div, LeftPar>;

    constexpr Visitor visitor_type {
        []([[maybe_unused]] Number i) -> type {return type::NUMBER;},
        []([[maybe_unused]] Plus i) -> type {return type::PLUS;},
        []([[maybe_unused]] Minus i) -> type {return type::MINUS;},
        []([[maybe_unused]] Mult i) -> type {return type::MULT;},
        []([[maybe_unused]] Div i) -> type {return type::DIV;},
        []([[maybe_unused]] LeftPar i) -> type {return type::LEFTPAR;}
    };

    constexpr Visitor precedence_visitor {
        []([[maybe_unused]] Number i) -> int {throw;},
        []([[maybe_unused]] Plus i) -> int {return 2;},
        []([[maybe_unused]] Minus i) -> int {return 2;},
        []([[maybe_unused]] Mult i) -> int {return 3;},
        []([[maybe_unused]] Div i) -> int {return 3;},
        []([[maybe_unused]] LeftPar i) -> int {return 0;}

    };

    constexpr int do_rpn(std::vector<rpn_variant> const &rpn)
    {
        std::vector<int> stack{};
        for (const auto part : rpn) {
            if (type t = std::visit(visitor_type, part); t == NUMBER) {
                stack.push_back(std::get<NUMBER>(part).value);
            } else {
                int right{stack.back()};
                stack.pop_back();
                int left{stack.back()};
                stack.pop_back();
                stack.push_back(std::visit([left, right](auto&& func){return func(left, right);}, part));
            }
        }
        return stack.front();
    }

    constexpr bool is_digit(char c) {
        return c <= '9' && c >= '0';
    }

    constexpr bool is_operator(char c) {
        return c == '+' || c == '-' || c == '*' || c == '/';
    }

    constexpr bool is_left_parenthesis(char c) {
        return c == '(';
    }

    constexpr bool is_right_parenthesis(char c) {
        return c == ')';
    }

    constexpr rpn_variant variant_from_char(char c) {
        if (c == '+') {
            return Plus{};
        } else if (c == '-') {
            return Minus{};
        } else if (c == '*') {
            return Mult{};
        } else if (c == '/') {
            return Div{};
        }
        throw;
    }

    constexpr int get_precedence(char c)
    {
        return (c == '+' || c == '-') ? 2 : (c == '*' || c == '/') ? 3 : 0;
    }

    template<FixedString STR>
    constexpr std::vector<rpn_variant> parse_str()
    {
        std::vector<rpn_variant> output{};
        std::vector<rpn_variant> stack{};

        for (std::size_t i = 0; i < STR.size(); i++) {
            if (is_digit(STR[i])) {
                int value = 0;
                for (; i < STR.size() && is_digit(STR[i]); i++) {
                    value = value * 10 + (STR[i] - '0');
                }
                output.push_back(rpn_variant{Number{value}});
                i--;
            } else if (is_operator(STR[i])) {
                while (stack.size() > 0 && std::visit(precedence_visitor, stack.back()) >= get_precedence(STR[i]) && std::visit(visitor_type, stack.back()) != type::LEFTPAR) {
                    output.push_back(stack.back());
                    stack.pop_back();
                }
                stack.push_back(variant_from_char(STR[i]));
            } else if (is_left_parenthesis(STR[i])) {
                stack.push_back(rpn_variant{LeftPar{}});
            } else if (is_right_parenthesis(STR[i])) {
                while (std::visit(visitor_type, stack.back()) != type::LEFTPAR) {
                    output.push_back(stack.back());
                    stack.pop_back();
                }
                stack.pop_back();
            }
        }
        while (!stack.empty()) {
            output.push_back(stack.back());
            stack.pop_back();
        }
        return output;
    }

    template<FixedString STR>
    consteval int get_result()
    {
        return do_rpn(parse_str<STR>());
    }
}

int main()
{
    return get_result<"6*((8+8)/2-1)">();
}
