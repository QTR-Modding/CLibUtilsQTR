// Author: Quantumyilmaz
// Year: 2025

#pragma once

namespace clib_utilsQTR {
    template<std::size_t S>
    struct PresetPool {
        size_t current = 0;
		std::array<std::string_view, S> names;
        explicit constexpr PresetPool(std::array<std::string_view, S> n) : names(n) {}
		size_t kTotal = S;
        std::string_view to_name(std::size_t idx) const {
            if (idx < kTotal) {
                return names[idx];
            }
            return "Unknown";
		}
    };

	template<typename T, std::size_t S, const PresetPool<S>& P>
    struct PresetSetting {
        T current;
        std::array<T, S> level_values;

        explicit PresetSetting(const std::array<T, S>& levels)
        : current(levels[0]), level_values(levels) {}

        explicit PresetSetting(const T& value)
        : current(value), level_values{make_level_values(value)} {}

        PresetSetting& operator=(const T& value) {
            current = value;
            return *this;
        }

        // ReSharper disable once CppNonExplicitConversionOperator
        operator const T&() const { return current; }
        // ReSharper disable once CppNonExplicitConversionOperator
        operator T&() { return current; }


        [[nodiscard]] const T& for_level(std::size_t lvl) const {return level_values[lvl];}
        void set_level(const std::size_t lvl) {current = level_values[lvl];}

        PresetSetting& operator=(const std::size_t lvl) {set_level(lvl);return *this;}

        [[nodiscard]] std::string_view name() const {
            return P.to_name(current_lvl());
		}

    private:
        constexpr std::array<T, S> make_level_values(T value) {
            std::array<T, S> result{};
            result.fill(value);
            return result;
        }
        [[nodiscard]] size_t current_lvl() const {
            for (size_t i = 0; i < S; ++i) {
                if (level_values[i] == current) {
                    return i;
                }
            }
            set_level(0);
            return 0;
		}
    };
}
