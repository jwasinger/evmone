// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2019-2020 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <benchmark/benchmark.h>
#include <evmc/evmc.hpp>
#include <evmone/analysis.hpp>
#include <limits>

namespace evmone::test
{
constexpr auto revision = EVMC_ISTANBUL;
constexpr auto gas_limit = std::numeric_limits<int64_t>::max();

extern evmc::VM vm;

inline evmc::result execute(bytes_view code, bytes_view input = {}) noexcept
{
    auto msg = evmc_message{};
    msg.gas = gas_limit;
    msg.input_data = input.data();
    msg.input_size = input.size();
    return vm.execute(revision, msg, code.data(), code.size());
}

inline void execute(benchmark::State& state, bytes_view code, bytes_view input = {}) noexcept
{
    auto total_gas_used = int64_t{0};
    auto iteration_gas_used = int64_t{0};
    for (auto _ : state)
    {
        auto r = execute(code, input);
        iteration_gas_used = gas_limit - r.gas_left;
        total_gas_used += iteration_gas_used;
    }

    using benchmark::Counter;
    state.counters["gas_used"] = Counter(static_cast<double>(iteration_gas_used));
    state.counters["gas_rate"] = Counter(static_cast<double>(total_gas_used), Counter::kIsRate);
}

inline void analyse(benchmark::State& state, bytes_view code) noexcept
{
    auto bytes_analysed = uint64_t{0};
    for (auto _ : state)
    {
        auto r = evmone::analyze(revision, code.data(), code.size());
        benchmark::DoNotOptimize(r);
        bytes_analysed += code.size();
    }

    using benchmark::Counter;
    state.counters["size"] = Counter(static_cast<double>(code.size()));
    state.counters["rate"] = Counter(static_cast<double>(bytes_analysed), Counter::kIsRate);
}
}  // namespace evmone::test
