#pragma once

#include <string>

#include <ctjson/Serializer.hpp>

#include <ctjson/detail/Field.hpp>

namespace ctjson {

/**
 * @brief Class to help implement serialization to json for custom types
 *
 * Usage example:
 * @code{.cpp}
 * struct DumpClass {
 *   std::string str;
 *   int integer;
 *
 *   template<typename Writer>
 *   static void json_dump(const DumpClass& object, Writer &writer) {
 *       auto str = SerializationHelper::Field("str", object.str); //
 * reference is saved here auto integer =
 * SerializationHelper::Field("integer", object.integer); // and here
 *       SerializationHelper::dump(writer, str, integer);
 *   }
 * };
 * @endcode
 */
class SerializationHelper {
public:
  /**
   * @brief Class representing class field
   *
   * @tparam T type of field
   */
  template <typename T>
  class Field : public detail::FieldBase<const T> {
    using Base = detail::FieldBase<const T>;

  public:
    /**
     * @param name name of filed - key in json
     * @param ref reference to value to dump
     */
    Field(std::string name, const T &ref) : Base(std::move(name), ref) {}

    /**
     * @return reference to value
     */
    const T &ref() const { return Base::m_ref; }
  };

  /**
   * @brief Dump class with given @param fields
   *
   * @tparam Writer type of writer
   * @tparam Args types of fields
   * @param writer writer
   * @param fields references to fields
   */
  template <typename Writer, typename... Args>
  static void dump(Writer &writer, Field<Args> &...fields) {
    writer.start_object();

    (
        [&]() {
          writer.key(fields.name);
          Serializer::dump<Args>(fields.ref(), writer);
        }(),
        ...);

    writer.end_object();
  }
};
} // namespace ctjson