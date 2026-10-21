# EDN Parser for C++

A modern, header-only C++ library for parsing and manipulating **Extensible Data Notation (EDN)** format. EDN is a data format used by Clojure that combines simplicity, expressiveness, and extensibility—perfect for configuration files, data exchange, and domain-specific languages.

## 🎯 Purpose

This library brings the elegance of EDN to C++, providing:

- **Full EDN specification compliance** - Parse all standard EDN data types
- **Type-safe representation** - Leverage C++'s strong type system with `std::variant`
- **Zero dependencies** - Header-only implementation using only the C++ standard library
- **Location-aware error reporting** - Get precise line and column information for parse errors
- **Pretty printing** - Beautiful, colorized output with configurable formatting
- **Evaluation engine** - Built-in support for evaluating EDN as a simple expression language

## ✨ Features

### Comprehensive Parsing
- Parse complete EDN syntax including nested structures
- Support for all primitive types, collections, and special forms
- Accurate error messages with line/column tracking
- Handles comments and whitespace correctly

### Rich Value System
- Type-safe `value_t` wrapper using modern C++ idioms
- Seamless conversion between C++ types and EDN values
- Deep equality comparison and ordering for all types
- Pattern matching via visitor pattern

### Pretty Printing & Formatting
- Configurable indentation and layout
- ANSI color support for terminal output
- Smart formatting for nested structures
- Compact or expanded output modes

### Expression Evaluation
- Evaluate EDN as executable code
- Symbol resolution with scoped environments
- Support for custom callable functions
- List evaluation for S-expressions

### Developer-Friendly API
- Intuitive constructors and type conversions
- Optional type checking via `if_*` methods
- Stream operators for easy I/O
- Modern C++17 features throughout

## 📦 Data Types

EDN supports a rich set of data types, all fully implemented in this library:

### Primitives

| Type | EDN Syntax | C++ Type | Description |
|------|-----------|----------|-------------|
| **nil** | `nil` | `nil_t` | Represents absence of value |
| **Boolean** | `true`, `false` | `boolean_t` (bool) | Boolean values |
| **Integer** | `42`, `-17`, `0` | `integer_t` (int) | Whole numbers |
| **Floating Point** | `3.14`, `-0.5`, `1e10` | `floating_point_t` (double) | Decimal numbers |
| **Character** | `\a`, `\newline`, `\space` | `character_t` (char) | Single characters with named literals |
| **String** | `"hello"`, `"world"` | `string_t` (std::string) | UTF-8 text strings |
| **Symbol** | `foo`, `my-var`, `+` | `symbol_t` | Identifiers used in code |
| **Keyword** | `:name`, `:age`, `:status` | `keyword_t` | Symbolic identifiers prefixed with `:` |

### Collections

| Type | EDN Syntax | C++ Type | Properties |
|------|-----------|----------|------------|
| **Vector** | `[1 2 3]` | `vector_t` | Ordered, indexed sequence |
| **List** | `(+ 1 2)` | `list_t` | Ordered sequence, used for code |
| **Set** | `#{1 2 3}` | `set_t` | Unordered, unique elements |
| **Map** | `{:a 1 :b 2}` | `map_t` | Key-value pairs, preserves insertion order |

### Special Forms

| Type | EDN Syntax | C++ Type | Description |
|------|-----------|----------|-------------|
| **Tagged Element** | `#inst "2024-01-01"` | `tagged_element_t` | Extensible tagged literals |
| **Quoted Element** | `'(1 2 3)` | `quoted_element_t` | Prevents evaluation |
| **Callable** | N/A | `callable_t` | First-class functions |

### Collection Features

**Vectors** are the workhorse collection type:
```cpp
vector_t v = {1, 2, 3, value_t("text"), true};
```

**Lists** are typically used for code representation:
```cpp
list_t expr = {symbol_t("+"), 1, 2, 3};  // Represents (+ 1 2 3)
```

**Sets** ensure uniqueness and support all value types:
```cpp
set_t unique = {1, 2, 3, 1, 2};  // Contains only {1 2 3}
```

**Maps** preserve insertion order and support heterogeneous keys:
```cpp
map_t person = {
    {keyword_t("name"), "Alice"},
    {keyword_t("age"), 30},
    {keyword_t("active"), true}
};
```

## 🚀 Quick Start

```cpp
#include <edn/edn.hpp>
#include <iostream>

int main() {
    // Parse EDN string
    auto data = edn::parse(R"(
        {:name "Alice"
         :age 30
         :hobbies ["reading" "coding" "hiking"]}
    )");

    // Pretty print with colors
    edn::pretty_print(std::cout, data);

    // Type-safe access
    if (auto map = data.if_map()) {
        auto name = map->at(edn::keyword_t("name"));
        std::cout << "Name: " << name << "\n";
    }

    return 0;
}
```

## 🔧 Usage Examples

### Parsing with Error Handling

```cpp
try {
    edn::value_t result = edn::parse("[1 2 3");
} catch (const edn::parse_error& e) {
    std::cerr << "Parse error: " << e.what() << "\n";
    // Error: Unterminated vector at line 1, column 6
}
```

### Building Data Structures

```cpp
using namespace edn;

// Create a complex nested structure
value_t config = map_t{
    {keyword_t("server"), map_t{
        {keyword_t("host"), "localhost"},
        {keyword_t("port"), 8080},
        {keyword_t("ssl"), true}
    }},
    {keyword_t("endpoints"), vector_t{
        "/api/users",
        "/api/posts",
        "/api/comments"
    }}
};
```

### Type Checking and Pattern Matching

```cpp
void process_value(const edn::value_t& value) {
    if (auto i = value.if_integer()) {
        std::cout << "Integer: " << *i << "\n";
    }
    else if (auto s = value.if_string()) {
        std::cout << "String: " << *s << "\n";
    }
    else if (auto v = value.if_vector()) {
        std::cout << "Vector with " << v->size() << " elements\n";
    }
}
```

### Expression Evaluation

```cpp
#include <edn/evaluate.hpp>

// Create an environment with the built-in arithmetic operators.
auto env = edn::make_standard_environment();

// Evaluate expressions
edn::value_t expr = edn::parse("(+ 1 2 3)");
edn::value_t result = edn::evaluate(expr, env);
// result is 6
```

The standard environment provides `+`, `-`, `*`, and `/`. Addition and
multiplication accept zero arguments and return their identity values; the
other operators require at least one argument. Integer arithmetic stays
integer-valued when possible and promotes to floating point on overflow.
Division returns an integer for exact integer quotients and floating point
otherwise.

It also provides the function combinators `partial` and `comp`:

```clojure
(def add5 (partial + 5))      ; binds leading arguments
(add5 1)                      ; => 6

(defn inc [x] (+ x 1))
((comp inc (partial * 2)) 5)  ; => 11, applied right to left
```

`(partial f)` and `(comp f)` return `f` itself, and `(comp)` returns the
identity function. The rightmost function given to `comp` receives all the
arguments; each function to its left receives the previous result.

#### Comparisons, predicates, and logic

The standard environment includes:

- `=`, `not=`: compare values using the library's existing, type-sensitive
  equality (including collection contents). For example, `(= 1 1.0)` is
  `false`. `not=` negates equality of all arguments; it does not require
  every pair to be different.
- `<`, `<=`, `>`, `>=`: numeric comparisons, allowing mixed integers and
  floating-point numbers. Multiple arguments compare adjacent pairs:
  `(< 1 2 3)` is `true`.
- Type predicates: `nil?`, `boolean?`, `true?`, `false?`, `number?`,
  `integer?`, `float?`, `char?`, `string?`, `symbol?`, `keyword?`,
  `list?`, `vector?`, `set?`, `map?`, and `fn?`.
- `coll?` recognizes lists, vectors, sets, and maps; `sequential?`
  recognizes lists and vectors.
- `not` negates truthiness.

Comparisons require at least one argument. A single-argument comparison
returns `true`, except `not=`, which returns `false`. Predicates and `not`
require exactly one argument. Invalid arities and nonnumeric arguments to
ordered comparisons raise errors.

Only `nil` and `false` are falsey; zero, empty strings, and empty collections
are truthy. This applies consistently to `if`, `cond`, `not`, and the
short-circuit special forms `and` and `or`. The special forms are available
even without the standard environment:

```clojure
(and true 42)          ; => 42: returns the first falsey value or the last value
(or nil false 0)       ; => 0: returns the first truthy value or the last value
(and false missing)    ; => false: missing is never evaluated
(or :ready missing)    ; => :ready
(and)                  ; => true
(or)                   ; => nil
```

#### Collection constructors and eager processing

The standard environment provides `vector`, `list`, `hash-map`, and
`hash-set`. Each accepts zero arguments to construct an empty collection.
Arguments are evaluated normally; lists preserve argument order, and sets
remove duplicates using the library's existing set semantics. `hash-map`
requires alternating keys and values (an even number of arguments).
Repeated keys keep their original position but use the last supplied value.
Despite their Clojure names, `hash-map` and `hash-set` construct the library's
existing map and set types, not new hash-based storage types.

```clojure
(vector 1 (+ 1 1))             ; => [1 2]
(list :a :b)                  ; => (:a :b)
(hash-map :a 1 :a 2)           ; => {:a 2}
(hash-set 1 2 1)               ; => #{1 2}
(mapv (partial + 1) '(1 2 3)) ; => [2 3 4]
(mapv + [1 2 3] [10 20])      ; => [11 22]
(filterv number? [1 :a 2])     ; => [1 2]
(reduce + 10 [1 2 3])         ; => 16
```

`mapv`, `filterv`, and `reduce` accept lists, vectors, or `nil` (an empty
input). Other input types raise an error.

- `(mapv f coll ...)` eagerly returns a vector, calling `f` with one element
  from each input, in order. It stops at the shortest input and requires at
  least one input collection.
- `(filterv pred coll)` eagerly returns a vector of elements whose predicate
  result is truthy, preserving order.
- `(reduce f coll)` folds left using the first element as the initial
  accumulator. For an empty input it calls `f` with no arguments; for a
  singleton input it returns that element without calling `f`.
- `(reduce f initial coll)` folds left starting with `initial`, returning it
  unchanged for an empty input.

These are ordinary functions and can be used with `partial` and `comp`.
Callback errors propagate to the caller. Lazy sequences, transducers, and
early termination via Clojure's `reduced` are not supported.

#### Scopes and lifetime

Scopes follow Clojure's rules, which keeps them free of reference cycles. Plain
reference counting therefore frees everything, and no garbage collector is
needed:

- `def` and `defn` always define in the root (global) environment, even
  inside a function body.
- `let` bindings are sequential, and a binding cannot refer to itself:
  `(let [f (fn [] (f))] ...)` fails with an unrecognized symbol. For
  recursion, use `defn`.
- Closures keep their local scopes (`let` bindings and function arguments)
  alive, so they can outlive the expression that created them.
- Closures look up globals in the root environment without owning it. Keep the
  environment alive while you use closures that refer to globals. Once the
  environment is destroyed, such lookups throw `std::logic_error`.

```cpp
edn::value_t f;
{
    auto env = edn::make_standard_environment();
    f = edn::evaluate(edn::parse("(let [x 1] (fn [] x))"), env);
}
(*f.if_callable())({});  // 1: uses only its own locals
```

## 📖 Advanced Features

### Custom Pretty Printing

```cpp
edn::pretty_print_options_t opts;
opts.indent_size = 4;
opts.max_inline_length = 40;
opts.compact_maps = false;

edn::pretty_print(std::cout, data, opts);
```

### Ordered Maps

Unlike standard C++ maps, EDN maps preserve insertion order:

```cpp
edn::map_t config = {
    {keyword_t("first"), 1},
    {keyword_t("second"), 2},
    {keyword_t("third"), 3}
};
// Iteration maintains insertion order
```

### Tagged Literals

```cpp
// Parse tagged elements
auto timestamp = edn::parse("#inst \"2024-01-01T00:00:00\"");

if (auto tagged = timestamp.if_tagged_element()) {
    std::cout << "Tag: " << tagged->tag() << "\n";
    std::cout << "Value: " << tagged->element() << "\n";
}
```

## 🏗️ Building

This is a header-only library. Simply include the headers:

```cpp
#include <edn/edn.hpp>
#include <edn/evaluate.hpp>  // Optional, for evaluation features
```

To build the examples and tests:

```bash
mkdir build && cd build
cmake ..
make
```

## 📝 License

See LICENSE file for details.

## 🤝 Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

---

*Built with modern C++17 • Fully type-safe • Zero dependencies*
