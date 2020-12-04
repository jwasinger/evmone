// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2019-2020 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0

#include "helpers.hpp"
#include "test/utils/bytecode.hpp"
#include <benchmark/benchmark.h>
#include <evmc/instructions.h>
#include <evmone/analysis.hpp>
#include <evmone/instruction_traits.hpp>
#include <iostream>

namespace evmone::test
{
using namespace benchmark;

namespace
{
constexpr auto stack_limit = 1023;

enum class Mode
{
    min_stack,
    full_stack,
};

const bytes loop_prefix = push(255) + OP_JUMPDEST;
const bytes loop_suffix = push("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff") +
                          OP_ADD + OP_DUP1 + push(2) + OP_JUMPI;

constexpr bool is_unary_op(evmc_opcode opcode) noexcept
{
    const auto trait = instr::traits[opcode];
    return trait.stack_height_required == 1 && trait.stack_height_change == 0;
}

constexpr bool is_binary_op(evmc_opcode opcode) noexcept
{
    const auto trait = instr::traits[opcode];
    return trait.stack_height_required == 2 && trait.stack_height_change == -1;
}

bytes_view generate_code(evmc_opcode opcode, Mode mode) noexcept
{
    static bytes cache[256][2]{};

    auto& code = cache[opcode][static_cast<int>(mode)];
    if (!code.empty())
        return code;

    bytes inner_code;
    switch (mode)
    {
    case Mode::min_stack:
    {
        if (opcode >= OP_PUSH1 && opcode <= OP_PUSH32)
        {
            bytes instr_pair;
            instr_pair.push_back(opcode);
            std::fill_n(std::back_inserter(instr_pair), opcode - OP_PUSH1 + 1, 0);
            instr_pair.push_back(OP_POP);
            for (int i = 0; i < stack_limit; ++i)
                inner_code += instr_pair;
        }
        else if (is_unary_op(opcode))
        {
            // Add the instruction twice to have the same instruction count in the loop as in the
            // other cases.
            inner_code.push_back(OP_DUP1);
            for (int i = 0; i < stack_limit; ++i)
            {
                inner_code.push_back(opcode);
                inner_code.push_back(opcode);
            }
            inner_code.push_back(OP_POP);
        }
        else if (is_binary_op(opcode))
        {
            inner_code.push_back(OP_DUP1);
            for (int i = 0; i < (stack_limit - 1); ++i)
            {
                inner_code.push_back(OP_DUP1);
                inner_code.push_back(opcode);
            }
            inner_code.push_back(OP_POP);
        }
        break;
    }
    case Mode::full_stack:
    {
        if (opcode >= OP_PUSH1 && opcode <= OP_PUSH32)
        {
            for (int i = 0; i < stack_limit; ++i)
            {
                inner_code.push_back(opcode);
                if (opcode >= OP_PUSH1 && opcode <= OP_PUSH32)
                    std::fill_n(std::back_inserter(inner_code), opcode - OP_PUSH1 + 1, 0);
            }
            std::fill_n(std::back_inserter(inner_code), stack_limit, OP_POP);
        }
        else if (is_binary_op(opcode))
        {
            std::fill_n(std::back_inserter(inner_code), stack_limit, OP_DUP1);
            std::fill_n(std::back_inserter(inner_code), stack_limit - 1, opcode);
            inner_code.push_back(OP_POP);
        }
        break;
    }
    }

    code = loop_prefix + inner_code + loop_suffix;
    return code;
}
}  // namespace

bool register_synthetic_benchmarks() noexcept
{
    std::vector<evmc_opcode> opcodes{OP_ADD, OP_SUB, OP_MUL, OP_ISZERO, OP_NOT};
    for (int i = OP_PUSH1; i <= OP_PUSH32; ++i)
        opcodes.push_back(static_cast<evmc_opcode>(i));

    RegisterBenchmark("execute/synth/loop", [](State& state) {
        const auto code = loop_prefix + loop_suffix;
        execute(state, code);
    })->Unit(kMicrosecond);

    for (auto opcode : opcodes)
    {
        const auto name = instr::traits[opcode].name;

        RegisterBenchmark((std::string{"execute/synth/"} + name + "/min_stack").c_str(),
            [opcode](State& state) { execute(state, generate_code(opcode, Mode::min_stack)); })
            ->Unit(kMicrosecond);

        if (!is_unary_op(opcode))
        {
            RegisterBenchmark((std::string{"execute/synth/"} + name + "/full_stack").c_str(),
                [opcode](State& state) { execute(state, generate_code(opcode, Mode::full_stack)); })
                ->Unit(kMicrosecond);
        }
    }

    if (std::getenv("BENCH_DUMP"))
    {
        std::cout << "PUSH16/min_stack:\n"
                  << hex(generate_code(OP_PUSH16, Mode::min_stack)) << "\n"
                  << "PUSH16/full_stack:\n"
                  << hex(generate_code(OP_PUSH16, Mode::full_stack)) << "\n";
    }

    return true;
}

[[maybe_unsed]] static const auto synthetic_benchmarks_registered = register_synthetic_benchmarks();
}  // namespace evmone::test
