import matplotlib.pyplot as plt

data_dir_path = "./data"
graph_dir_path = "./"


def args_to_file_name(
    k, d, traffic_pattern, routing_algo, degree, picking_algo
):
    prefix = "k" + str(k) + "d" + str(d) + "_" + traffic_pattern
    if routing_algo == 0:
        return prefix + "_deter.txt"
    elif routing_algo == 1:
        return prefix + "_sta_" + str(degree) + ".txt"
    else:
        return (
            prefix + "_dyn_" + str(degree) + "_" + str(picking_algo) + ".txt"
        )


def axis_name(op):
    axisname = [
        "injrate",
        "injpac",
        "recpac",
        "qlat",
        "nlat",
        "lat",
        "hop",
        "dr",
        "misroute",
        "recrate",
    ]
    return axisname[op]


def axis_label(op):
    axislabel = [
        "Injection Rate (Packets/Cycle/Node)",
        "Packets Injected (Packets)",
        "Packets Received (Packets)",
        "Average Queuing Latency (Cycle)",
        "Average Network Latency (Cycle)",
        "Average Latency (Cycle)",
        "Average Hops",
        "Average Dimension Reversals",
        "Average Misrouting Times",
        "Reception Rate",
    ]
    return axislabel[op]


def plot_graph_from_file(fig, filenames, labels, linecolors, x_axis, y_axis):
    fig.tick_params(axis="both", labelsize=18, direction="in")
    for op in range(len(filenames)):
        with open(data_dir_path + "/" + filenames[op], "r") as file:
            lines = file.readlines()
        line_no = 0
        x_data = []
        y_data = []
        # hyperparam = int(lines[line_no])
        # line_no += 1
        while line_no in range(len(lines)):
            xp = float(lines[line_no + x_axis])
            yp = float(lines[line_no + y_axis])
            if 3 <= x_axis <= 5:
                xp /= 50
            if 3 <= y_axis <= 5:
                yp /= 50
            line_no += 10
            x_data.append(xp)
            y_data.append(yp)
        if len(x_data) > 0:
            fig.plot(
                x_data,
                y_data,
                color=linecolors[op],
                label=labels[op],
            )


def compare_algo(fig, traffic_pattern, x_axis, y_axis):
    filenames = [
        args_to_file_name(8, 2, traffic_pattern, 0, -1, -1),
        args_to_file_name(8, 2, traffic_pattern, 1, 1, -1),
        args_to_file_name(8, 2, traffic_pattern, 2, 0, 1),
    ]
    labels = ["Deterministic", "Static Adaptive", "Dynamic Adaptive"]
    colors = ["r", "g", "b"]
    plot_graph_from_file(fig, filenames, labels, colors, x_axis, y_axis)


def compare_throttling(fig, traffic_pattern, x_axis, y_axis):
    filenames = [
        args_to_file_name(8, 2, traffic_pattern, 2, 0, 1),
        args_to_file_name(8, 2, traffic_pattern, 2, 4, 1),
    ]
    labels = ["No Throttling", "4 vcs"]
    colors = ["r", "b"]
    plot_graph_from_file(fig, filenames, labels, colors, x_axis, y_axis)


def compare_pick(fig, traffic_pattern, x_axis, y_axis):
    filenames = [
        args_to_file_name(8, 2, traffic_pattern, 2, 0, 0),
        args_to_file_name(8, 2, traffic_pattern, 2, 0, 1),
        args_to_file_name(8, 2, traffic_pattern, 2, 0, 2),
    ]
    labels = ["Random", "Minimum Congestion", "Straight Lines"]
    colors = ["r", "g", "b"]
    plot_graph_from_file(fig, filenames, labels, colors, x_axis, y_axis)


def compare_dim(fig, traffic_pattern, x_axis, y_axis):
    filenames = [
        args_to_file_name(8, 2, traffic_pattern, 2, 0, 1),
        args_to_file_name(4, 3, traffic_pattern, 2, 0, 1),
    ]
    labels = ["8-ary 2-cube torus", "4-ary 3-cube torus"]
    colors = ["r", "b"]
    plot_graph_from_file(fig, filenames, labels, colors, x_axis, y_axis)


def draw_fig(traffic_name, y_axis_op, save_name, compare_function):
    fig1, axs = plt.subplots(
        len(y_axis_op),
        len(traffic_name),
        figsize=(10 * len(traffic_name), 8 * len(y_axis_op)),
        sharex="col",
    )
    for row in range(len(y_axis_op)):
        for col in range(len(traffic_name)):
            compare_function(
                axs[row][col], traffic_name[col], 0, y_axis_op[row]
            )
    for row in range(len(y_axis_op)):
        axs[row][0].set_ylabel(axis_label(y_axis_op[row]), fontsize=20)
    for col in range(len(traffic_name)):
        axs[len(y_axis_op) - 1][col].set_xlabel(axis_label(0), fontsize=20)
        axs[0][col].set_title(traffic_name[col], fontsize=20)
    lines, labels = axs[-1][-1].get_legend_handles_labels()
    fig1.legend(
        lines, labels, loc="lower center", ncol=3, fontsize=24, frameon=False
    )
    fig1.tight_layout(pad=7)
    fig1.savefig(f"{graph_dir_path}/{save_name}", bbox_inches="tight")


if __name__ == "__main__":
    draw_fig(
        ["uniform_random", "bit_reverse", "bit_rotation", "transpose"],
        [4, 5, 6, 9],
        "algo.png",
        compare_algo,
    )
    draw_fig(
        ["uniform_random", "bit_reverse", "bit_rotation", "bit_complement"],
        [4, 6, 9],
        "dim.png",
        compare_dim,
    )
    draw_fig(
        ["uniform_random", "bit_rotation", "transpose"],
        [4, 5, 7, 8, 9],
        "throttling.png",
        compare_throttling,
    )
    draw_fig(
        ["uniform_random", "bit_reverse", "bit_rotation", "transpose"],
        [4, 6, 7, 8],
        "pick.png",
        compare_pick,
    )
