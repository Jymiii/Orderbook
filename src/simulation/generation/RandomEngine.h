#pragma once

#include <array>
#include <random>

class RandomEngine
{
public:
    explicit RandomEngine(const std::mt19937::result_type seed = std::random_device{}())
        : rng_{seed}
    {
    }

    std::mt19937& engine() noexcept { return rng_; }

    double normal() noexcept { return normalDist_(rng_); }
    double uniformSpread() noexcept { return uniformSpreadDist_(rng_); }
    double uniform01() noexcept { return uniform01Dist_(rng_); }
    bool coinflip() noexcept { return bernoulliDist_(rng_); }

    int poisson(const double mean)
    {
        std::poisson_distribution<int> d{mean};
        return d(rng_);
    }

    template <typename Int>
    Int uniformInt(Int lo, Int hi)
    {
        std::uniform_int_distribution<Int> d{lo, hi};
        return d(rng_);
    }

private:
    std::mt19937 rng_;
    std::normal_distribution<double> normalDist_{0.0, 1.0};
    std::uniform_real_distribution<double> uniformSpreadDist_{-0.499999999, 0.5};
    std::uniform_real_distribution<double> uniform01Dist_{0.0, 1.0};
    std::bernoulli_distribution bernoulliDist_{0.5};
};
