# Adapted from https://github.com/cmuparlay/flock/blob/ae/create_graphs.py

import matplotlib as mpl

mpl.rcParams["grid.linestyle"] = ":"
mpl.rcParams.update({"font.size": 20})

import matplotlib.pyplot as plt
import statistics as st
import os

operations = {
    "mwobject": ["update"],
    "arrayswap": ["update"],
    "stack": ["update"],
    "queue": ["update"],
    "deque": ["update"],
    "sorted-list": ["read", "update", "mixed"],
    "hashmap": ["read", "update", "mixed"],
    "bst": ["read", "update", "mixed"],
}

REF_ALG = "lock"

sync_types = [
    REF_ALG,
    "lockfree",
    "lockfree-mcas",
    "lockfree-flock",
    "mcas-no-if",
    "mcas-htm",
    "mcas-htm-no-if",
    "htm-lock",
    "htm-mcas",
]


class DSInfo:
    def __init__(self, color, marker, linestyle, name):
        self.color = color
        self.marker = marker
        self.linestyle = linestyle
        self.name = name


mk = [
    "o",
    "v",
    "^",
    "1",
    "s",
    "+",
    "x",
    "D",
    "|",
    ">",
    "<",
]

dsinfo = {
    "lock": DSInfo("C0", mk[0], "-", "lock-based"),
    "lockfree": DSInfo("C1", mk[1], "-", "lock-free CAS"),
    "lockfree-mcas": DSInfo("C2", mk[2], "-", "MCAS con lock"),
    "lockfree-flock": DSInfo("C3", mk[3], "-", "flock"),
    "mcas-no-if": DSInfo("C6", mk[6], "-", "MCAS sin if"),
    "mcas-htm": DSInfo("C7", mk[7], "-", "MCAS con HTM"),
    "mcas-htm-no-if": DSInfo("C8", mk[8], "-", "MCAS con HTM sin if"),
    "htm-lock": DSInfo("C4", mk[4], "-", "HTM sobre lock"),
    "htm-mcas": DSInfo("C5", mk[5], "-", "HTM sobre MCAS"),
}


def toString(algname, th, ops):
    return algname + "-" + str(th) + "t-" + str(ops) + "o"


# statistics.fmean() was added in Python 3.8, this can be used instead
def avg(numlist):
    total = 0.0
    length = 0
    for num in numlist:
        length = length + 1
        total += float(num)
    if length > 0:
        return 1.0 * total / length
    else:
        return -1


def read_results_file(
    filename, results, stddev, threads, algs, read_time, warm_up_rounds
):
    results_raw = {}

    ds = ""
    th = ""
    op_type = ""
    ops = ""
    time = ""

    with open(filename, "r", encoding="utf-8") as file:
        for line in file.readlines():
            line = line.strip()
            if line.find("type:") != -1:
                op_type = line.split(" ")[1]
            elif line.find("datastructure:") != -1:
                ds = line.split(" ")[1]
            elif line.find("threads: ") != -1:
                th = int(line.split(" ")[1])
            elif line.find("ops: ") != -1:
                ops = int(line.split(" ")[1])
            elif line.find("time(us): ") != -1:
                time = int(line.split(" ")[1])
            elif line.find("operation end") != -1:
                if th not in threads:
                    threads.append(th)

                alg = ds + "_" + op_type
                if alg not in algs:
                    algs.append(alg)

                key = toString(alg, th, ops)
                if key not in results_raw:
                    results_raw[key] = []
                if read_time:
                    results_raw[key].append(float(time) / 1000.0)
                else:
                    results_raw[key].append(float(ops) / float(time))

        for key in results_raw:
            valid_runs = results_raw[key][warm_up_rounds:]
            results[key] = avg(valid_runs)
            stddev[key] = st.pstdev(valid_runs)


def export_legend(legend, filename="legend.pdf"):
    fig = legend.figure
    fig.canvas.draw()
    bbox = legend.get_window_extent().transformed(fig.dpi_scale_trans.inverted())
    fig.savefig(filename, dpi="figure", bbox_inches=bbox)


def plot_scalability_graph(
    op,
    throughput,
    stddev,
    threads,
    n_ops,
    data_struct,
    graph_name,
    paper_ver,
    plot_time,
    error_bars,
    bin_name,
    normalize,
):
    graphtitle = graph_name + "-" + op + "-" + str(n_ops) + "ops"
    graphs_dir = "graphs/" + bin_name
    if not os.path.exists(graphs_dir):
        os.makedirs(graphs_dir)

    if paper_ver:
        output_file = graphs_dir + "/" + graphtitle.replace(".", "") + ".pdf"
        mpl.rcParams.update({"font.size": 25})
    else:
        output_file = graphs_dir + "/" + graphtitle + ".png"

    print("plotting " + output_file)

    series = {}
    error = {}
    reference = throughput[toString(REF_ALG + "_" + data_struct + "_" + op, 1, n_ops)]
    for alg in sync_types:
        alg_id = alg + "_" + data_struct + "_" + op
        series[alg_id] = []
        error[alg_id] = []
        for th in threads:
            key = toString(alg_id, th, n_ops)
            if key not in throughput:
                if series[alg_id]:
                    del series[alg_id]
                if error[alg_id]:
                    del error[alg_id]
                continue
            if normalize:
                series[alg_id].append(throughput[key] / reference)
                error[alg_id].append(stddev[key] / reference)
            else:
                series[alg_id].append(throughput[key])
                error[alg_id].append(stddev[key])

    _, axs = plt.subplots()
    axs.set_xscale("log", base=2)
    axs.set_yscale("log", base=10)

    for alg in sync_types:
        alginfo = dsinfo[alg]
        alg_id = alg + "_" + data_struct + "_" + op
        if alg_id not in series:
            continue
        if not series[alg_id]:
            continue
        plot_params = dict(
            alpha=0.8,
            color=alginfo.color,
            linewidth=3.0,
            linestyle=alginfo.linestyle,
            marker=alginfo.marker,
            markersize=14,
            label=alginfo.name,
        )
        if error_bars:
            axs.errorbar(
                threads, series[alg_id], yerr=error[alg_id], capsize=5, **plot_params
            )
        else:
            axs.plot(threads, series[alg_id], **plot_params)

    plt.grid()

    axs.set(xlabel="Número de hilos")
    if plot_time:
        ylabel = "Tiempo"
        if normalize:
            ylabel += " normalizado"
            axs.set(ylabel=ylabel)
        else:
            ylabel += " (ms)"
            axs.set(ylabel=ylabel)
    else:
        ylabel = "Rendimiento"
        if normalize:
            ylabel += " normalizado"
            axs.set(ylabel=ylabel)
        else:
            ylabel += " (Mop/s)"
            axs.set(ylabel=ylabel)

    if not paper_ver:
        plt.title(graphtitle)
        plt.legend(loc="center left", bbox_to_anchor=(1, 0.5))

    plt.savefig(output_file, bbox_inches="tight")

    if paper_ver:
        legend = plt.legend(
            loc="center left",
            bbox_to_anchor=(1, 0.5),
            ncol=3,
            framealpha=0.0,
        )
        export_legend(legend, graphs_dir + "/" + graph_name + "_legend.pdf")

    plt.close("all")
