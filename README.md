# Compile time calculator
This is a compile time calculator based on some new features of C++20 (constexpr std::vector).

You can compile it with GCC 12, or MSVC 19.29, but currently clang doesn't support constexpr vectors.

```c++
int res = get_result<"6*((8+8)/2-1)", int>();
```
