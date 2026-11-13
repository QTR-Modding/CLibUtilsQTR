#pragma once
#include <SKSE/SKSE.h>
#include <functional>
#include <limits>
#include <type_traits>
#include <unordered_map>
#include <map>
#include <mutex>

namespace Serialization {
    // Credits: https:// github.com/ozooma10/OSLAroused/blob/29ac62f220fadc63c829f6933e04be429d4f96b0/src/PersistedData.cpp
    // BaseData is based off how powerof3's did it in Afterlife
    template <typename T, typename U>
    class BaseData {
    public:
        float GetData(T formId, T missing) {
            Locker locker(m_Lock);
            // if the plugin version is less than 0.7 need to handle differently
            // if (SKSE::PluginInfo::version)
            if (auto idx = m_Data.contains(formId)) {
                return m_Data[formId];
            }
            return missing;
        }

        void SetData(T formId, U value) {
            Locker locker(m_Lock);
            m_Data[formId] = value;
        }

        virtual const char* GetType() = 0;

        virtual bool Save(SKSE::SerializationInterface*, std::uint32_t,
                          std::uint32_t) { return false; }

        virtual bool Save(SKSE::SerializationInterface*) { return false; }
        virtual bool Load(SKSE::SerializationInterface*) { return false; }

        void Clear() {
            Locker locker(m_Lock);
            m_Data.clear();
        }

    protected:
        ~BaseData() = default;
        std::map<T, U> m_Data;

        using Lock = std::recursive_mutex;
        using Locker = std::lock_guard<Lock>;
        mutable Lock m_Lock;
    };

    inline std::vector<std::pair<int, bool>> encodeString(const std::string& inputString) {
        std::vector<std::pair<int, bool>> encodedValues;
        try {
            for (int i = 0; i < 100 && inputString[i] != '\0'; i++) {
                const char ch = inputString[i];
                if (std::isprint(ch) && (std::isalnum(ch) || std::isspace(ch) || std::ispunct(ch)) && ch >= 0) {
                    encodedValues.emplace_back(static_cast<int>(ch), std::isupper(ch));
                }
            }
        } catch (const std::exception& e) {
            SKSE::log::error("Error encoding string: {}", e.what());
            return encodeString("ERROR");
        }
        return encodedValues;
    }

    inline std::string decodeString(const std::vector<std::pair<int, bool>>& encodedValues) {
        std::string decodedString;
        for (const auto& pair : encodedValues) {
            const char ch = static_cast<char>(pair.first);
            if (std::isalnum(ch) || std::isspace(ch) || std::ispunct(ch)) {
                if (pair.second) {
                    decodedString += ch;
                } else {
                    decodedString += static_cast<char>(std::tolower(ch));
                }
            }
        }
        return decodedString;
    }

    inline bool read_string(SKSE::SerializationInterface* a_intfc, std::string& a_str) {
        std::vector<std::pair<int, bool>> encodedStr;
        std::size_t size;
        if (a_intfc->ReadRecordData(size) != sizeof(size)) {
            return false;
        }
        for (std::size_t i = 0; i < size; i++) {
            std::pair<int, bool> temp_pair;
            if (a_intfc->ReadRecordData(temp_pair) != sizeof(temp_pair)) {
                return false;
            }
            encodedStr.push_back(temp_pair);
        }
        a_str = decodeString(encodedStr);
        return true;
    }

    inline bool write_string(SKSE::SerializationInterface* a_intfc, const std::string& a_str) {
        const auto encodedStr = encodeString(a_str);
        // i first need the size to know no of iterations
        const auto size = encodedStr.size();
        if (!a_intfc->WriteRecordData(size)) {
            return false;
        }
        for (const auto& temp_pair : encodedStr) {
            if (!a_intfc->WriteRecordData(temp_pair)) {
                return false;
            }
        }
        return true;
    };
}

namespace Serialization::V2 {
    enum class Type { Number, String, FormID };

    template <class T>
    concept Field = std::is_arithmetic_v<T> || std::is_same_v<T, std::string>;

    template <class T>
    concept MapContainer =
        std::is_same_v<T, std::map<typename T::key_type, typename T::mapped_type,
                                  typename T::key_compare, typename T::allocator_type>> ||
        std::is_same_v<T, std::unordered_map<typename T::key_type, typename T::mapped_type,
                                            typename T::hasher, typename T::key_equal, typename T::allocator_type>>;

    class Registry {
    public:
        template <Serialization::V2::Field T, class Allocator>
        bool Register(std::uint32_t tag, std::uint32_t version, std::vector<T, Allocator>& data, Type type);

        template <Serialization::V2::MapContainer Map>
            requires (Field<typename Map::key_type> && Field<typename Map::mapped_type>)
        bool Register(std::uint32_t tag, std::uint32_t version, Map& data, Type keyType, Type valueType);

        bool Save(const SKSE::SerializationInterface* skse) const;
        bool Load(const SKSE::SerializationInterface* skse) const;
        void Clear() const;

    private:
        struct Record {
            std::uint32_t version;
            std::function<void()> clear;
            std::function<bool(const SKSE::SerializationInterface*)> save;
            std::function<bool(const SKSE::SerializationInterface*, std::uint32_t)> load;
        };

        template <class T>
        static constexpr bool Supports(Type type);
        template <class T>
        static bool Write(const SKSE::SerializationInterface* skse, const T& value, std::uint32_t& remaining);
        template <class T>
        static bool Read(const SKSE::SerializationInterface* skse, T& value, std::uint32_t& remaining);
        template <class T>
        static bool Resolve(const SKSE::SerializationInterface* skse, T& value, Type type);
        template <class Container>
        bool RegisterContainer(std::uint32_t tag, std::uint32_t version, Container& data, Type type, Type valueType = Type::Number);
        template <class Container>
        static bool SaveContainer(const SKSE::SerializationInterface* skse, const Container& data);
        template <class Container>
        static bool LoadContainer(const SKSE::SerializationInterface* skse, Container& data,
                                  std::uint32_t remaining, Type type, Type valueType);

        std::map<std::uint32_t, Record> records;
    };
}

template <Serialization::V2::Field T, class Allocator>
bool Serialization::V2::Registry::Register(std::uint32_t tag, std::uint32_t version,
                                          std::vector<T, Allocator>& data, Type type) {
    return Supports<T>(type) && RegisterContainer(tag, version, data, type);
}

template <Serialization::V2::MapContainer Map>
    requires (Serialization::V2::Field<typename Map::key_type> && Serialization::V2::Field<typename Map::mapped_type>)
bool Serialization::V2::Registry::Register(std::uint32_t tag, std::uint32_t version, Map& data,
                                          Type keyType, Type valueType) {
    return Supports<typename Map::key_type>(keyType) && Supports<typename Map::mapped_type>(valueType) &&
           RegisterContainer(tag, version, data, keyType, valueType);
}

template <class T>
constexpr bool Serialization::V2::Registry::Supports(Type type) {
    switch (type) {
    case Type::Number:
        return std::is_arithmetic_v<T>;
    case Type::String:
        return std::is_same_v<T, std::string>;
    case Type::FormID:
        return std::is_same_v<T, RE::FormID>;
    }
    return false;
}

template <class T>
bool Serialization::V2::Registry::Write(const SKSE::SerializationInterface* skse, const T& value,
                                       std::uint32_t& remaining) {
    if constexpr (std::is_same_v<T, std::string>) {
        if (value.size() > remaining || remaining - value.size() < sizeof(std::uint32_t)) {
            return false;
        }
        const auto length = static_cast<std::uint32_t>(value.size());
        remaining -= length;
        return Write(skse, length, remaining) && (!length || skse->WriteRecordData(value.data(), length));
    } else if constexpr (std::is_same_v<T, bool>) {
        return Write(skse, static_cast<std::uint8_t>(value), remaining);
    } else {
        if (sizeof(T) > remaining) {
            return false;
        }
        remaining -= sizeof(T);
        return skse->WriteRecordData(value);
    }
}

template <class T>
bool Serialization::V2::Registry::Read(const SKSE::SerializationInterface* skse, T& value,
                                      std::uint32_t& remaining) {
    if constexpr (std::is_same_v<T, std::string>) {
        std::uint32_t length;
        if (!Read(skse, length, remaining) || length > remaining) {
            return false;
        }
        value.resize(length);
        return !length || skse->ReadRecordDataEx(remaining, value.data(), length) == length;
    } else if constexpr (std::is_same_v<T, bool>) {
        std::uint8_t byte;
        if (!Read(skse, byte, remaining) || byte > 1) {
            return false;
        }
        value = byte != 0;
        return true;
    } else {
        return sizeof(T) <= remaining && skse->ReadRecordDataEx(remaining, value) == sizeof(T);
    }
}

template <class T>
bool Serialization::V2::Registry::Resolve(const SKSE::SerializationInterface* skse, T& value, Type type) {
    if constexpr (std::is_same_v<T, RE::FormID>) {
        return type != Type::FormID || skse->ResolveFormID(value, value);
    } else {
        return true;
    }
}

template <class Container>
bool Serialization::V2::Registry::RegisterContainer(std::uint32_t tag, std::uint32_t version,
                                                   Container& data, Type type, Type valueType) {
    return records.try_emplace(tag, Record{
        version,
        [&data] { data.clear(); },
        [&data](const SKSE::SerializationInterface* skse) { return SaveContainer(skse, data); },
        [&data, type, valueType](const SKSE::SerializationInterface* skse, std::uint32_t length) {
            return LoadContainer(skse, data, length, type, valueType);
        }
    }).second;
}

template <class Container>
bool Serialization::V2::Registry::SaveContainer(const SKSE::SerializationInterface* skse, const Container& data) {
    auto remaining = std::numeric_limits<std::uint32_t>::max();
    if (data.size() > remaining || !Write(skse, static_cast<std::uint32_t>(data.size()), remaining)) {
        return false;
    }
    for (const auto& entry : data) {
        if constexpr (MapContainer<Container>) {
            if (!Write(skse, entry.first, remaining) || !Write(skse, entry.second, remaining)) {
                return false;
            }
        } else {
            if (!Write<typename Container::value_type>(skse, entry, remaining)) {
                return false;
            }
        }
    }
    return true;
}

template <class Container>
bool Serialization::V2::Registry::LoadContainer(const SKSE::SerializationInterface* skse, Container& data,
                                               std::uint32_t remaining, Type type, Type valueType) {
    std::uint32_t count;
    if (!Read(skse, count, remaining)) {
        return false;
    }

    auto loaded = [&data] {
        if constexpr (!MapContainer<Container>) {
            return Container(data.get_allocator());
        } else if constexpr (requires { data.key_comp(); }) {
            return Container(data.key_comp(), data.get_allocator());
        } else {
            Container result(0, data.hash_function(), data.key_eq(), data.get_allocator());
            result.max_load_factor(data.max_load_factor());
            return result;
        }
    }();
    for (std::uint32_t i = 0; i < count; ++i) {
        if constexpr (MapContainer<Container>) {
            typename Container::key_type key;
            typename Container::mapped_type value;
            if (!Read(skse, key, remaining) || !Read(skse, value, remaining)) {
                return false;
            }
            if (Resolve(skse, key, type) && Resolve(skse, value, valueType)) {
                loaded.insert_or_assign(std::move(key), std::move(value));
            }
        } else {
            typename Container::value_type value;
            if (!Read(skse, value, remaining)) {
                return false;
            }
            if (Resolve(skse, value, type)) {
                loaded.push_back(std::move(value));
            }
        }
    }
    if (remaining != 0) {
        return false;
    }
    data.swap(loaded);
    return true;
}

inline bool Serialization::V2::Registry::Save(const SKSE::SerializationInterface* skse) const {
    bool success = true;
    for (const auto& [tag, record] : records) {
        if (!skse->OpenRecord(tag, record.version) || !record.save(skse)) {
            success = false;
        }
    }
    return success;
}

inline bool Serialization::V2::Registry::Load(const SKSE::SerializationInterface* skse) const {
    Clear();
    bool success = true;
    std::uint32_t tag, version, length;
    while (skse->GetNextRecordInfo(tag, version, length)) {
        const auto entry = records.find(tag);
        if (entry == records.end() || entry->second.version != version) {
            continue;
        }
        const auto& record = entry->second;
        if (!record.load(skse, length)) {
            success = false;
        }
    }
    return success;
}

inline void Serialization::V2::Registry::Clear() const {
    for (const auto& [tag, record] : records) {
        record.clear();
    }
}
