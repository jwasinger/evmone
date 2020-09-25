// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#include "execution.hpp"
#include "analysis.hpp"
#include <iostream>
#include <memory>

namespace evmone
{
evmc_result execute(evmc_vm* /*unused*/, const evmc_host_interface* host, evmc_host_context* ctx,
    evmc_revision rev, const evmc_message* msg, const uint8_t* code, size_t code_size) noexcept
{
    auto analysis = analyze(rev, code, code_size);

    auto state = std::make_unique<execution_state>();
    state->analysis = &analysis;
    state->msg = msg;
    state->code = code;
    state->code_size = code_size;
    state->host = evmc::HostContext{*host, ctx};
    state->gas_left = msg->gas;
    state->rev = rev;

    const auto* instr = &state->analysis->instrs[0];
    while (instr != nullptr)
        instr = instr->fn(instr, *state);

    const auto gas_left =
        (state->status == EVMC_SUCCESS || state->status == EVMC_REVERT) ? state->gas_left : 0;

    // print the number of mulmods, addmods and submods

    /*
    std::cout << "mulmodcount: " << mulmodmont_count << "\naddmod count: " << addmod_count << "\nsubmod count: " << submod_count << "\n\n";
    mulmodmont_count = 0;
    addmod_count = 0;
    submod_count = 0;
    */

    return evmc::make_result(
        state->status, gas_left, &state->memory[state->output_offset], state->output_size);
}
}  // namespace evmone
