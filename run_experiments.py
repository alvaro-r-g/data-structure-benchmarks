# Adapted rom https://github.com/cmuparlay/flock/blob/ae/run_experiments.py

import argparse
import os

from create_graphs import (
    sync_types,
    operations,
    read_results_file,
    plot_scalability_graph,
)

ROUNDS = 20
WARM_UP_ROUNDS = 5

parser = argparse.ArgumentParser()
parser.add_argument(
    "datastructure",
    help="[mwobject, arrayswap, stack, queue, deque, sorted-list, hashmap, bst]",
)
parser.add_argument("threads")
parser.add_argument("n_ops")
parser.add_argument("sync", help='sync types, can be "all", "not-mcas" or a list')
parser.add_argument("-g", "--graphs_only", action="store_true")
parser.add_argument("-e", "--error_bars", action="store_true")
parser.add_argument("-n", "--normalize", action="store_true")
parser.add_argument(
    "-p",
    "--paper_ver",
    help="paper version of graphs, no title or legends",
    action="store_true",
)
parser.add_argument(
    "-t",
    "--time",
    help="plot time instead of throughput",
    action="store_true",
)
parser.add_argument("-b", "--bin", default="build/data_structure_benchmarks")

args = parser.parse_args()

datastructure = args.datastructure
threads = args.threads
n_ops = args.n_ops
sync = args.sync
graphs_only = args.graphs_only
plot_time = args.time
paper_ver = args.paper_ver
error_bars = args.error_bars
bin_path = args.bin
normalize = args.normalize


def string_to_list(s):
    s = s.strip().strip("[").strip("]").split(",")
    return [ss.strip() for ss in s]


def to_list(s):
    if isinstance(s, list):
        return s
    return [s]


def runstring(s, a, op, out):
    os.system('echo "' + op + '"')
    os.system('echo "' + op + '" >> ' + out)
    os.system('echo "datastructure: ' + s + "_" + a + '" >> ' + out)
    for _ in range(ROUNDS):
        x = os.system(op + " >> " + out)
        if x:
            if os.WEXITSTATUS(x) == 0:
                raise NameError("  aborted: " + op)
            os.system("echo Failed")


def runtest(s, a, procs, num_ops, out):
    runstring(
        s,
        a,
        # + " numactl -i all "
        "./"
        + bin_path
        + " -s "
        + s
        + " -n "
        + str(procs)
        + " -a "
        + a
        + " -o "
        + str(num_ops),
        out,
    )


print("datastructure: " + datastructure)
print("n_ops: " + n_ops)

if "[" in args.threads:
    threads = string_to_list(threads)

if sync == "all":
    sync = sync_types
elif sync == "not-mcas":
    sync = ["lock", "lockfree", "lockfree-flock", "htm-lock", "htm-mcas"]
else:
    sync = string_to_list(sync)

bin_name = os.path.basename(os.path.normpath(bin_path))
results_dir = "results/" + bin_name
if not graphs_only and not os.path.exists(results_dir):
    os.makedirs(results_dir)

outfile = (
    results_dir
    + "/"
    + "-".join([args.datastructure, args.threads, args.n_ops])
    + ".txt"
)

if not graphs_only:
    # clear output file
    os.system('echo "" > ' + outfile)
    for sy in sync:
        for th in to_list(threads):
            runtest(sy, datastructure, th, n_ops, outfile)

throughput = {}
stddev = {}
threads = []
algs = []

read_results_file(outfile, throughput, stddev, threads, algs, plot_time, WARM_UP_ROUNDS)
threads.sort()

print("threads: " + str(threads))
print("algs: " + str(algs))

for operation in operations[datastructure]:
    plot_scalability_graph(
        operation,
        throughput,
        stddev,
        threads,
        n_ops,
        datastructure,
        datastructure,
        paper_ver,
        plot_time,
        error_bars,
        bin_name,
        normalize,
    )
