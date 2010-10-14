// THIS IS PARTIALLY IMPLEMENTED AND LIKELY OBSOLETE

// This set of classes represents the *abstract* proto type system.
// This is different than the instantiated type system.

// Every type has a null of some sort

// Scalar
// Number
// Tuple[X,Y,...]
// Homogeneous tuple
// Null
// Vector[X]
// Field[X]

// What types are concrete?
// Scalar (& integer & boolean), [Tuple or lambda or field] of concrete
// What types are abstract?
// Any, Null, Local, Number, [Tuple or lambda or field] of abstract

// Instantiated types: constants of scalar, integer, boolean, tuple
// Variable types: non-constants

// Atomic types: scalar, integer, boolean
// Compound types: field, tuple, lambda
// Unresolved types: any, null, local, number

// Types need to be able to:
// - determine instanceof relations
// - refine towards agreement

// Coercion (only among concrete types)
// Scalar -> Vector(1)
// Local -> Field(Local)
// [Lambda: ??? -> Y] -> [Lambda: ??? -> Coercion(Y)]

// Lambda -> Lambda<i,j> -> instantiated lambdas
// Lambda lattice:
// two zones: inputs, outputs
// Knowledge available: # of elements in zone, types of elements
// no knowledge > # of elements, unknown type > known type

// A lambda is also concrete if it can be variable-bound

// LISP-style specialized lambda lists: args &optional | &rest
// Need to be able to have *variables* in lambdas also.
// For example, FOLD has the sig:  FOLD: [Lambda: X,Y -> X] X [LIST Y] -> X

ANY: ABSTRACT, VARIABLE, UNRESOLVED
  NULL ISA ANY
  LOCAL ISA ANY
  NUMBER ISA ANY
  FIELD ISA ANY: COMPOUND

  type->empty() is the "bottom" element, representing a conflict

namespace ProtoType {
  using namespace std;
  
  typedef enum {
    Any, // Root
    Null, Field, Local, // First layer
    Number, Lambda, Tuple, // Second layer
    HomoTuple, Vector, Scalar, Integer, Boolean // Further descendants
  } TypeName;
  
  class Type { // Root type
  public:
    /* Type property functions: */
    virtual TypeName name() { return Any; }
    
    // Concrete types have a particular representation (e.g. Scalar)
    // while abstract types do not (e.g. Number)
    virtual bool concrete() { return false; }
    bool abstract() { return !concrete(); }
  
    // Instantiated types have an actual value (e.g. "3" or "true")
    // while variable types do not (e.g. Integer
    virtual bool instantiated() { return false; }
    bool variable() { return !instantiated(); }
    
    // Atomic types have no substructure to be described (e.g. Scalar) while
    // compound types do (e.g. Vector has a length) and unresolved types
    // are abstracts whose atomicity can't be determined
    virtual bool atomic() { return false; }
    virtual bool compound() { return false; }
    bool unresolved() { return !(atomic() || compound()); }

    // Type manipulation functions
    boal isa(Type* t);  // This type is the same as or a subtype of t
    bool coercable(Type* t); // This type can be coerced to a subtype of t
    static Type* intersect(Type* a, Type* b); // return the max type that isa A and B
  };

  class NullType : public Type {
  };

  class FieldType : public Type {
    bool atomic() { return false; }
    bool compound() { return true; }
  };
  class LocalType : public Type {
  };

  class NumberType : public LocalType {
  };
  class LambdaType : public LocalType {
    bool atomic() { return false; } bool compound() { return true; }
    vector<Type*> 
  };

  // (T1 T2 .. TK . Trest)
  class TupleType : public LocalType {
    bool atomic() { return false; } bool compound() { return true; }
    vector<Type*> start;
    Type* rest;
    vector<Type*> values;
  };
  class VectorType : public HTupleType, public NumberType {
    bool concrete() { return true; }
  };
  class ScalarType : public NumberType {
    bool has_value;
    bool concrete() { return true; }
    bool instantiated() { return has_value; }
    bool atomic() { return true; } bool compound() { return false; }
    float value;
  };
  class IntegerType : public ScalarType {
    int value;
  };
  class BooleanType : public IntegerType {
    bool value;
  };

}

// Every concrete type has an instantiated type that is its null
// A program is executable iff everything resolves to concrete types
