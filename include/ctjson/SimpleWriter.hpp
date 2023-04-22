#pragma once

#include <rapidjson/writer.h>

#include <ctjson/detail/TypeUtils.hpp>

namespace ctjson {

template <typename OutputStream> class SimpleWriter {
  public:
    SimpleWriter(OutputStream &os) : m_writer(os) {}

    bool is_complete() const { return m_writer.IsComplete(); }

    void null() { m_writer.Null(); }

    void boolean(bool value) { m_writer.Bool(value); }

    template <typename Int> void integer(Int value) {
        if (value > 0) {
            if (detail::in_range<unsigned>(value)) {
                m_writer.Uint(value);
            } else {
                m_writer.Uint64(value);
            }
        } else {
            if (detail::in_range<int>(value)) {
                m_writer.Int(value);
            } else {
                m_writer.Int64(value);
            }
        }
    }

    template <typename Floating> void floating(Floating value) {
        m_writer.Double(value);
    }

    void string(const std::string &value) {
        m_writer.String(value.data(), value.length(), true);
    }

    void start_object() { m_writer.StartObject(); }

    void key(const std::string &key) {
        m_writer.Key(key.data(), key.length(), true);
    }

    void end_object() { m_writer.EndObject(); }

    void start_array() { m_writer.StartArray(); }

    void end_array() { m_writer.EndArray(); }

  private:
    rapidjson::Writer<OutputStream> m_writer;
};
} // namespace ctjson