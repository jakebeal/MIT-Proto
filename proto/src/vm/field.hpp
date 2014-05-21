/*   ____       _  __ _   ____            _
 *  |  _ \  ___| |/ _| |_|  _ \ _ __ ___ | |_ ___
 *  | | | |/ _ \ | |_| __| |_) | '__/ _ \| __/ _ \
 *  | |_| |  __/ |  _| |_|  __/| | ( (_) | |( (_) )
 *  |____/ \___|_|_|  \__|_|   |_|  \___/ \__\___/
 *
 * This file is part of DelftProto.
 * See COPYING for license details.
 */

/// \file
/// Provides the FieldData class

#ifndef __FIELD_HPP
#define __FIELD_HPP

#include "types.hpp"
#include "machineid.hpp"


class Data;

/** \class FieldData
 * \brief Essentially a SharedVector of MachineId,Data pairs.
 * 
 * One of the types that can be stored in Data.
 */

class FieldData {
protected:
  SharedVector<MachineId> ids;
  SharedVector<Data> values;

public:
  //  class iterator {
  // }

public:
  /// Construct an empty field.
  inline FieldData() {}
  
  /// Allocate a new field with the specified initial capacity.
  inline explicit FieldData(Size capacity) : ids(capacity), values(capacity) {}
  
  /// Construct another instance of this field
  /**
   * \note The contents will be shared. Use copy() to get a copy.
   */
  inline FieldData(FieldData const & source) : ids(source.ids), values(source.values) {}
  
  /// Add a pair to the back of the field.
  inline void push(MachineId const & source, Data const & value) {
    ids.push(source); values.push(value);
  }
  
  /// Get the contents from another field.
  /**
   * \note The contents will be shared, not copied. To get a copy, you can use:
   * \code
   * vector_a = vector_b.copy();
   * \endcode
   */
  inline FieldData & operator = (FieldData const & source) {
    ids = source.ids; values = source.values;
    return *this;
  }
  
  /// Create a copy of this field
  /**
   * All elements will be copied using their own copy constructor.
   */
  inline FieldData copy() const {
    FieldData f = FieldData(size());
    f.ids = ids.copy(); f.values = values.copy();
    return f;
  }
  
  /// The number of elements in this field
  inline Size size() const {
    return values.size();
  }
  
  /// Check whether the field is empty (true) or not (false).
  inline bool empty() const {
    return size() == 0;
  }
  
  /// The field current capacity.
  inline Size capacity() const {
    return values.capacity();
  }
  
  /// The number of instances with the same shared contents including this one.
  inline Counter instances() const {
    return values.instances();
  }
  
  /// Deconstruct the field.
  /**
   * If this was the last instance of this vector, the contents will be deconstructed and deallocated as well.
   */
  inline ~FieldData() {
    // shared vectors should be automatically deconstructed
  }
};

#endif
