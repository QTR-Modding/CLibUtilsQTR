#pragma once
#include <type_traits>
#include <vector>
#include <string>
#include <rapidjson/document.h>

namespace Presets {

    namespace detail {
        template <typename T>
        struct is_std_vector : std::false_type {};

        template <typename T, typename A>
        struct is_std_vector<std::vector<T, A>> : std::true_type {};

        template <typename T>
        inline constexpr bool is_std_vector_v = is_std_vector<T>::value;
    }

    namespace Getters {

        template<class>
        inline constexpr bool always_false_v = false;

        namespace JSON {

            template <typename T>
            bool GetScalar(const rapidjson::Value& val, T& out) {
                if constexpr (std::is_same_v<T, std::string>) {
                    if (val.IsString()) {
                        out = val.GetString();
                        return true;
                    }
                }
                else if constexpr (std::is_same_v<T, float>) {
                    if (val.IsFloat()) {
                        out = val.GetFloat();
                        return true;
                    }
                }
                else if constexpr (std::is_same_v<T, double>) {
                    if (val.IsDouble()) {
                        out = val.GetDouble();
                        return true;
                    }
                }
                else if constexpr (std::is_same_v<T, std::int64_t>) {
                    if (val.IsInt64()) {
                        out = val.GetInt64();
                        return true;
                    }
                }
                else if constexpr (std::is_same_v<T, std::uint64_t>) {
                    if (val.IsUint64()) {
                        out = val.GetUint64();
                        return true;
                    }
                }
                else if constexpr (std::is_same_v<T, std::int32_t> || std::is_same_v<T, int>) {
                    if (val.IsInt()) {
                        out = val.GetInt();
                        return true;
                    }
                }
                else if constexpr (std::is_same_v<T, std::uint32_t>) {
                    if (val.IsUint()) {
                        out = val.GetUint();
                        return true;
                    }
                }
                else if constexpr (std::is_same_v<T, bool>) {
                    if (val.IsBool()) {
                        out = val.GetBool();
                        return true;
                    }
                }
                else {
                    static_assert(always_false_v<T>, "Unsupported scalar type");
                }

                return false;
            }

            template <typename T>
            bool Get(const rapidjson::Value& val, T& a_to_set) {
                if constexpr (detail::is_std_vector_v<T>) {
                    using ElemType = typename T::value_type;

                    if (!val.IsArray()) return false;
                    const auto& arr = val.GetArray();
                    a_to_set.reserve(arr.Size());

                    for (const auto& item : arr) {
                        ElemType temp;
                        if (!GetScalar(item, temp)) return false;
                        a_to_set.push_back(std::move(temp));
                    }

                    return true;

                } else {
                    return GetScalar(val, a_to_set);
                }
            }


            template <typename T>
            bool Get(const rapidjson::Value& obj, const std::string& key, T& out) {
                if (!obj.IsObject() || !obj.HasMember(key.c_str())) return false;
                const auto& val = obj[key.c_str()];
                return Get(val, out);
            }


        }

        template <typename T, typename BlockType>
        bool GetValue(const BlockType& a_block, std::string& a_name, T& a_val) {
            T result;
            if constexpr (std::is_same_v<BlockType, rapidjson::Value>) {
                if (Presets::Getters::JSON::Get<T>(a_block,a_name,result)) {
                    a_val = result;
                    return true;
                }
            } else {
                // Add other types as needed
            }
            return false;
        }
    }

}