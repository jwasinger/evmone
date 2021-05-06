import nanodurationpy
import json

def geth_data() -> {}:
    geth_lines = []
    with open("geth_bench_output.log", "r") as f:
        geth_lines = f.read()

    geth_traces = geth_lines.split("0x")
    geth_traces = [[val for val in trace.split('\n') if val] for trace in geth_traces if trace][:-1]

    geth_traces = [(trace[0].split('/')[1], trace[2][17:]) for trace in geth_traces]

    result = {}
    for trace in geth_traces:
            result[trace[0]] = nanodurationpy.from_str(trace[1]).total_seconds() * 1000

    return result

def evmone_data() -> ({}, {}):
    evmone_benchmarks = None
    with open("evmone_bench_output.log") as f:
        evmone_benchmarks = json.load(f)

    baseline_result = {}
    advanced_result = {}

    for b in evmone_benchmarks['benchmarks']:
        name = b['name']

        vm = name.split('/')[0]
        benchmark = name.split('/')[3:]

        if len(benchmark) > 1:
            benchmark = benchmark[0] + "_" + benchmark[1]
        else:
            benchmark = benchmark[0]

        elapsed = nanodurationpy.from_str(str(b['cpu_time']) + b['time_unit'])

        if vm == 'advanced':
            advanced_result[benchmark] = elapsed.total_seconds() * 1000
        else:
            baseline_result[benchmark] = elapsed.total_seconds() * 1000

    return (baseline_result, advanced_result)

geth_trace_result = geth_data()
baseline_trace_result, advanced_trace_result = evmone_data()

assert set(geth_trace_result.keys()).difference(set(baseline_trace_result.keys())) == set()
assert set(geth_trace_result.keys()).difference(set(advanced_trace_result.keys())) == set()

print("benchmark,geth-evm,evmone-baseline,evmone-advanced")
for bench_name in geth_trace_result.keys():
    print("{}, {}, {}, {}".format(bench_name, geth_trace_result[bench_name], baseline_trace_result[bench_name], advanced_trace_result[bench_name]))
