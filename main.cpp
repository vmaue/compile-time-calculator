#include <variant>
#include <vector>
#include <type_traits>

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
 
    template<typename T> struct Number {T value;
        T operator()([[maybe_unused]] T a, [[maybe_unused]] T b) const { throw; }};
    template<typename T> struct Plus{constexpr T operator()(T a, T b) const { return a + b; }};
    template<typename T> struct Minus{constexpr T operator()(T const a, T const b) const {
        if constexpr (std::is_unsigned<T>::value) {
            if (b > a) {
                throw;
            }
        }
        return a - b;
    }};
    template<typename T> struct Mult{constexpr T operator()(T a, T b) const { return a * b; }};
    template<typename T> struct Div{constexpr T operator()(T a, T b) const { return a / b; }};
    template<typename T> struct LeftPar{T operator()([[maybe_unused]] T a, [[maybe_unused]] T b) const { throw; }};

    enum type {
        NUMBER,
        PLUS,
        MINUS,
        MULT,
        DIV,
        LEFTPAR
    };

    template<typename T>
    using rpn_variant = std::variant<Number<T>, Plus<T>, Minus<T>, Mult<T>, Div<T>, LeftPar<T>>;

    template<typename T>
    constexpr Visitor visitor_type {
        []([[maybe_unused]] Number<T> i) -> type {return type::NUMBER;},
        []([[maybe_unused]] Plus<T> i) -> type {return type::PLUS;},
        []([[maybe_unused]] Minus<T> i) -> type {return type::MINUS;},
        []([[maybe_unused]] Mult<T> i) -> type {return type::MULT;},
        []([[maybe_unused]] Div<T> i) -> type {return type::DIV;},
        []([[maybe_unused]] LeftPar<T> i) -> type {return type::LEFTPAR;}
    };

    template<typename T>
    constexpr Visitor precedence_visitor {
        []([[maybe_unused]] Number<T> i) -> int {throw;},
        []([[maybe_unused]] Plus<T> i) -> int {return 2;},
        []([[maybe_unused]] Minus<T> i) -> int {return 2;},
        []([[maybe_unused]] Mult<T> i) -> int {return 3;},
        []([[maybe_unused]] Div<T> i) -> int {return 3;},
        []([[maybe_unused]] LeftPar<T> i) -> int {return 0;}
    };

    template<typename T>
    constexpr T do_rpn(std::vector<rpn_variant<T>> const &rpn)
    {
        std::vector<T> stack{};
        for (const auto part : rpn) {
            if (type t = std::visit(visitor_type<T>, part); t == NUMBER) {
                stack.push_back(std::get<NUMBER>(part).value);
            } else {
                T const right{stack.back()};
                stack.pop_back();
                T const left{stack.back()};
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

    template<typename T>
    constexpr rpn_variant<T> variant_from_char(char c) {
        if (c == '+') {
            return Plus<T>{};
        } else if (c == '-') {
            return Minus<T>{};
        } else if (c == '*') {
            return Mult<T>{};
        } else if (c == '/') {
            return Div<T>{};
        }
        throw;
    }

    constexpr int get_precedence(char c)
    {
        return (c == '+' || c == '-') ? 2 : (c == '*' || c == '/') ? 3 : 0;
    }

    template<FixedString STR, typename T>
    constexpr std::vector<rpn_variant<T>> parse_str()
    {
        std::vector<rpn_variant<T>> output{};
        std::vector<rpn_variant<T>> stack{};

        for (std::size_t i = 0; i < STR.size(); i++) {
            if (is_digit(STR[i])) {
                T value = 0;
                for (; i < STR.size() && is_digit(STR[i]); i++) {
                    value = value * 10 + static_cast<T>(STR[i] - '0');
                }
                output.push_back(rpn_variant<T>{Number<T>{value}});
                i--;
            } else if (is_operator(STR[i])) {
                while (!stack.empty() && std::visit(precedence_visitor<T>, stack.back()) >= get_precedence(STR[i]) && std::visit(visitor_type<T>, stack.back()) != type::LEFTPAR) {
                    output.push_back(stack.back());
                    stack.pop_back();
                }
                stack.push_back(variant_from_char<T>(STR[i]));
            } else if (is_left_parenthesis(STR[i])) {
                stack.push_back(rpn_variant<T>{LeftPar<T>{}});
            } else if (is_right_parenthesis(STR[i])) {
                while (std::visit(visitor_type<T>, stack.back()) != type::LEFTPAR) {
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

    template<FixedString STR, typename T>
    consteval T get_result()
    {
        return do_rpn<T>(parse_str<STR, T>());
    }
}

int main()
{
    return get_result<"6*((8+8)/2-1)", int>();
}
