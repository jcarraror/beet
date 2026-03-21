# Beet Language Specification (v0)

---

## Binding

bind name = value
mutable name = value

bind name is Type = value
mutable name is Type = value

name = value

---

## Type Relation

name is Type

---

## Functions

function name(param is Type, ...) returns Type {
    return value
}

---

## Types

type Name = TypeExpression

type Name = structure {
    field is Type
}

type Name(Parameter) = TypeExpression

type Option(Value) = choice {
    none
    some(Value)
}

---

## Control Flow

if condition {
    ...
} else {
    ...
}

while condition {
    ...
}

match value {
    case pattern {
        ...
    }
}

---

## Expressions

name(value, ...)

Type(field = value, ...)

Type.variant()
Type.variant(value)

Point(x = 3, y = 4)
Option.some(1)
Option.none()

---

## Generics

Option(Int)
Outcome(Value, Error)
Lookup(Key, Value)

---

## Blocks

{
    ...
}

---

## Ownership

borrowed Type
borrowed mutable Type
owned Type

function length(point is borrowed Point) returns Float

---

# Design Rules

- code must be readable without decoding
- words carry meaning, symbols carry structure
- one concept, one form
- explicit over implicit
- bind introduces names
- is expresses type relation
- syntax must not affect runtime performance
- avoid historical baggage
- prefer stability over cleverness