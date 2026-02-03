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

// Create an environment with built-in functions
edn::stack_t env{nullptr};

// Evaluate expressions
edn::value_t expr = edn::parse("(+ 1 2 3)");
edn::value_t result = edn::evaluate(expr, env);
// result is 6
```

## 📖 Advanced Features

### Custom Pretty Printing

```cpp
edn::pretty_print_options opts;
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
